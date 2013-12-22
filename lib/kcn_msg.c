#include <sys/queue.h>

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

#define KCN_MSG_HDRSIZ	(1 /* version */ + 1 /* type */ + 2 /* length*/)

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
	kcn_pkt_put16(kp, kcn_pkt_len(kp));
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
	if (kmh->kmh_len > KCN_MSG_MAXSIZ) {
		KCN_LOG(ERR, "recv too long message, %zu bytes", kmh->kmh_len);
		errno = E2BIG;
		goto bad;
	}
	if (kcn_pkt_len(kp) < kmh->kmh_len) {
		KCN_LOG(DEBUG, "recv partial message");
		kcn_pkt_start(kp);
		errno = EAGAIN;
		goto bad;
	}
	return true;
  bad:
	return false;
}

void
kcn_msg_query_encode(struct kcn_pkt *kp, const struct kcn_msg_query *kmq)
{
	const struct kcn_formula *kf = &kmq->kmq_formula;

	kcn_msg_pkt_init(kp);
	kcn_pkt_put8(kp, kmq->kmq_maxcount);
	kcn_pkt_put8(kp, kf->kf_type);
	kcn_pkt_put8(kp, kf->kf_op);
	kcn_pkt_put64(kp, kf->kf_val);
	kcn_msg_header_encode(kp, KCN_MSG_TYPE_QUERY);
}

bool
kcn_msg_query_decode(struct kcn_pkt *kp, struct kcn_msg_query *kmq)
{
	struct kcn_formula *kf;

#define KCN_MSG_QUERY_MINSIZ	(1 + 1 + 1 + 8)
	if (kcn_pkt_trailingdata(kp) < KCN_MSG_QUERY_MINSIZ) {
		errno = EINVAL; /* XXX */
		goto bad;
	}
	kmq->kmq_maxcount = kcn_pkt_get8(kp);
	kf = &kmq->kmq_formula;
	kf->kf_type = kcn_pkt_get8(kp);
	kf->kf_op = kcn_pkt_get8(kp);
	kf->kf_val = kcn_pkt_get64(kp);
	return true;
  bad:
	return false;
}

void
kcn_msg_response_init(struct kcn_msg_response *kmr, struct kcn_info *ki)
{

	kmr->kmr_error = 0;
	kmr->kmr_leftcount = 0;
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
kcn_msg_response_decode(struct kcn_pkt *kp, struct kcn_msg_response *kmr)
{
	struct kcn_info *ki = kmr->kmr_ki;

#define KCN_MSG_RESPONSE_MINSIZ		(1 + 1)
	if (kcn_pkt_trailingdata(kp) < KCN_MSG_RESPONSE_MINSIZ) {
		errno = EINVAL; /* XXX */
		goto bad;
	}
	kmr->kmr_error = kcn_pkt_get8(kp);
	kmr->kmr_leftcount = kcn_pkt_get8(kp);
	if (kcn_info_maxnlocs(ki) - kcn_info_nlocs(ki) < kmr->kmr_leftcount) {
		KCN_LOG(ERR, "too many responses, %u", kmr->kmr_leftcount);
		errno = ETOOMANYREFS; /* XXX */
		goto bad;
	}

	if (! kcn_info_loc_add(kmr->kmr_ki, kcn_pkt_current(kp),
	    kcn_pkt_trailingdata(kp)))
		goto bad;

	kcn_pkt_reset(kp, 0);
	if (kmr->kmr_leftcount != 0) {
		errno = EAGAIN;
		goto bad;
	}
	return true;
  bad:
	return false;
}