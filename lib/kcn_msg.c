#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_pkt.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_msg.h"

static void
kcn_msg_pkt_init(struct kcn_pkt *kp)
{

	kcn_pkt_reset(kp, KCN_MSG_HDRSIZ);
}

static void
kcn_msg_header_encode(struct kcn_pkt *kp, enum kcn_msg_type type)
{

	kcn_pkt_prepend(kp, KCN_MSG_HDRSIZ);
	kcn_pkt_put8(kp, KCN_MSG_VERSION);
	kcn_pkt_put8(kp, type);
	kcn_pkt_put16(kp, kcn_pkt_len(kp) - KCN_MSG_HDRSIZ);
}

bool
kcn_msg_header_decode(struct kcn_pkt *kp, struct kcn_msg_header *kmh)
{

	if (kcn_pkt_trailingdata(kp) < KCN_MSG_HDRSIZ) {
		KCN_LOG(DEBUG, "recv partial header");
		errno = EAGAIN;
		goto bad;
	}
	kmh->kmh_version = kcn_pkt_get8(kp);
	kmh->kmh_type = kcn_pkt_get8(kp);
	kmh->kmh_len = kcn_pkt_get16(kp);

	if (kmh->kmh_version != KCN_MSG_VERSION) {
		KCN_LOG(ERR, "recv version mismatch %u (local) and %u (remote)",
		    KCN_MSG_VERSION, kmh->kmh_version);
		errno = EOPNOTSUPP;
		goto bad;
	}
	if (kmh->kmh_len > KCN_MSG_MAXBODYSIZ) {
		KCN_LOG(ERR, "recv too long message, %zu bytes", kmh->kmh_len);
		errno = E2BIG;
		goto bad;
	}
	if (kcn_pkt_trailingdata(kp) < kmh->kmh_len) {
		KCN_LOG(DEBUG, "recv partial message");
		kcn_pkt_start(kp);
		errno = EAGAIN;
		goto bad;
	}
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	return true;
  bad:
	return false;
}

void
kcn_msg_query_encode(struct kcn_pkt *kp, const struct kcn_msg_query *kmq)
{
	const struct kcn_formula *kf = &kmq->kmq_formula;

	kcn_msg_pkt_init(kp);
	kcn_pkt_put8(kp, kmq->kmq_loctype);
	kcn_pkt_put8(kp, kmq->kmq_maxcount);
	kcn_pkt_put64(kp, kmq->kmq_time);
	kcn_pkt_put8(kp, kf->kf_type);
	kcn_pkt_put8(kp, kf->kf_op);
	kcn_pkt_put64(kp, kf->kf_val);
	kcn_msg_header_encode(kp, KCN_MSG_TYPE_QUERY);
}

bool
kcn_msg_query_decode(struct kcn_pkt *kp, const struct kcn_msg_header *kmh,
    struct kcn_msg_query *kmq)
{
	struct kcn_formula *kf;

	if (kcn_pkt_trailingdata(kp) < KCN_MSG_QUERY_SIZ ||
	    kmh->kmh_len != KCN_MSG_QUERY_SIZ) {
		errno = EINVAL; /* XXX */
		goto bad;
	}
	kmq->kmq_loctype = kcn_pkt_get8(kp);
	kmq->kmq_maxcount = kcn_pkt_get8(kp);
	kmq->kmq_time = kcn_pkt_get64(kp);
	kf = &kmq->kmq_formula;
	kf->kf_type = kcn_pkt_get8(kp);
	kf->kf_op = kcn_pkt_get8(kp);
	kf->kf_val = kcn_pkt_get64(kp);
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	switch (kmq->kmq_loctype) {
	case KCN_LOC_TYPE_DOMAINNAME:
	case KCN_LOC_TYPE_URI:
		break;
	default:
		errno = EINVAL; /* XXX */
		goto bad;
	}
	return true;
  bad:
	return false;
}

void
kcn_msg_response_init(struct kcn_msg_response *kmr, struct kcn_info *ki)
{

	kmr->kmr_error = 0;
	kmr->kmr_leftcount = 0;
	kmr->kmr_score = 0;
	kmr->kmr_ki = ki;
}

void
kcn_msg_response_encode(struct kcn_pkt *kp, const struct kcn_msg_response *kmr)
{
	const struct kcn_info *ki;
	const char *loc;
	size_t idx;

	kcn_msg_pkt_init(kp);
	kcn_pkt_put8(kp, kmr->kmr_error);
	kcn_pkt_put8(kp, kmr->kmr_leftcount);
	kcn_pkt_put8(kp, kmr->kmr_score);

	ki = kmr->kmr_ki;
	if (ki != NULL && kcn_info_nlocs(ki) > 0) {
		assert(kcn_info_nlocs(ki) > kmr->kmr_leftcount);
		idx = kcn_info_nlocs(ki) - kmr->kmr_leftcount - 1;
		loc = kcn_info_loc(ki, idx);
		assert(loc != NULL);
		kcn_pkt_put(kp, loc, strlen(loc));
	}

	kcn_msg_header_encode(kp, KCN_MSG_TYPE_RESPONSE);
}

bool
kcn_msg_response_decode(struct kcn_pkt *kp, const struct kcn_msg_header *kmh,
    struct kcn_msg_response *kmr)
{
	struct kcn_info *ki = kmr->kmr_ki;
	size_t len = kmh->kmh_len;

	if (kcn_pkt_trailingdata(kp) < KCN_MSG_RESPONSE_MINSIZ ||
	    len < KCN_MSG_RESPONSE_MINSIZ) {
		errno = EINVAL; /* XXX */
		goto bad;
	}
	kmr->kmr_error = kcn_pkt_get8(kp);
	kmr->kmr_leftcount = kcn_pkt_get8(kp);
	kmr->kmr_score = kcn_pkt_get8(kp);
	len -= KCN_MSG_RESPONSE_MINSIZ;
	kcn_pkt_trim_head(kp, kcn_pkt_headingdata(kp));
	if (kcn_info_maxnlocs(ki) - kcn_info_nlocs(ki) < kmr->kmr_leftcount) {
		KCN_LOG(ERR, "too many responses, %u", kmr->kmr_leftcount);
		errno = ETOOMANYREFS; /* XXX */
		goto bad;
	}

	if (! kcn_info_loc_add(kmr->kmr_ki, kcn_pkt_current(kp), len,
	    kmr->kmr_score))
		goto bad;
	kcn_pkt_trim_head(kp, len);

	if (kmr->kmr_leftcount != 0) {
		errno = EAGAIN;
		goto bad;
	}
	return true;
  bad:
	return false;
}

void
kcn_msg_add_encode(struct kcn_pkt *kp, const struct kcn_msg_add *kma)
{

	kcn_msg_pkt_init(kp);
	kcn_pkt_put64(kp, kma->kma_time);
	kcn_pkt_put64(kp, kma->kma_val);
	kcn_pkt_put(kp, kma->kma_loc, kma->kma_loclen);
	kcn_msg_header_encode(kp, KCN_MSG_TYPE_QUERY);
}

bool
kcn_msg_add_decode(struct kcn_pkt *kp, const struct kcn_msg_header *kmh,
    struct kcn_msg_add *kma)
{

	if (kcn_pkt_trailingdata(kp) < KCN_MSG_ADD_MINSIZ ||
	    kmh->kmh_len < KCN_MSG_ADD_MINSIZ) {
		errno = EINVAL;
		goto bad;
	}
	kma->kma_time = kcn_pkt_get64(kp);
	kma->kma_val = kcn_pkt_get64(kp);
	kma->kma_loc = kcn_pkt_current(kp);
	kma->kma_loclen = kmh->kmh_len - KCN_MSG_ADD_MINSIZ;
	kcn_pkt_trim_head(kp, kmh->kmh_len);
	return true;
  bad:
	return false;
}
