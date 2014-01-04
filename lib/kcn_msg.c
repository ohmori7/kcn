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
#include "kcn_buf.h"
#include "kcn_eq.h"
#include "kcn_msg.h"

static void
kcn_msg_pkt_init(struct kcn_buf *kb)
{

	kcn_buf_reset(kb, KCN_MSG_HDRSIZ);
}

static void
kcn_msg_header_encode(struct kcn_buf *kb, enum kcn_msg_type type)
{

	kcn_buf_prepend(kb, KCN_MSG_HDRSIZ);
	kcn_buf_put8(kb, KCN_MSG_VERSION);
	kcn_buf_put8(kb, type);
	kcn_buf_put16(kb, kcn_buf_len(kb) - KCN_MSG_HDRSIZ);
	kcn_buf_dump(kb, kcn_buf_len(kb));
}

bool
kcn_msg_header_decode(struct kcn_buf *kb, struct kcn_msg_header *kmh)
{

	if (kcn_buf_trailingdata(kb) < KCN_MSG_HDRSIZ) {
		KCN_LOG(DEBUG, "recv partial header");
		errno = EAGAIN;
		goto bad;
	}
	kmh->kmh_version = kcn_buf_get8(kb);
	kmh->kmh_type = kcn_buf_get8(kb);
	kmh->kmh_len = kcn_buf_get16(kb);
	kcn_buf_dump(kb, KCN_MSG_HDRSIZ +
	    min(kmh->kmh_len, kcn_buf_trailingdata(kb)));

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
	if (kcn_buf_trailingdata(kb) < kmh->kmh_len) {
		KCN_LOG(DEBUG, "recv partial message");
		kcn_buf_start(kb);
		errno = EAGAIN;
		goto bad;
	}
	kcn_buf_trim_head(kb, kcn_buf_headingdata(kb));
	return true;
  bad:
	return false;
}

void
kcn_msg_query_encode(struct kcn_buf *kb, const struct kcn_msg_query *kmq)
{
	const struct kcn_eq *ke = &kmq->kmq_eq;

	kcn_msg_pkt_init(kb);
	kcn_buf_put8(kb, kmq->kmq_loctype);
	kcn_buf_put8(kb, kmq->kmq_maxcount);
	kcn_buf_put8(kb, ke->ke_type);
	kcn_buf_put8(kb, ke->ke_op);
	kcn_buf_put64(kb, ke->ke_val);
	kcn_buf_put64(kb, ke->ke_start);
	kcn_buf_put64(kb, ke->ke_end);
	kcn_msg_header_encode(kb, KCN_MSG_TYPE_QUERY);
}

bool
kcn_msg_query_decode(struct kcn_buf *kb, const struct kcn_msg_header *kmh,
    struct kcn_msg_query *kmq)
{
	struct kcn_eq *ke;

	if (kcn_buf_trailingdata(kb) < KCN_MSG_QUERY_SIZ ||
	    kmh->kmh_len != KCN_MSG_QUERY_SIZ) {
		errno = EINVAL; /* XXX */
		goto bad;
	}
	kmq->kmq_loctype = kcn_buf_get8(kb);
	kmq->kmq_maxcount = kcn_buf_get8(kb);
	ke = &kmq->kmq_eq;
	ke->ke_type = kcn_buf_get8(kb);
	ke->ke_op = kcn_buf_get8(kb);
	ke->ke_val = kcn_buf_get64(kb);
	ke->ke_start = kcn_buf_get64(kb);
	ke->ke_end = kcn_buf_get64(kb);
	kcn_buf_trim_head(kb, kcn_buf_headingdata(kb));
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
kcn_msg_response_init(struct kcn_msg_response *kmr)
{

	kmr->kmr_error = 0;
	kmr->kmr_score = 0;
	kmr->kmr_loc = NULL;
	kmr->kmr_loclen = 0;
}

void
kcn_msg_response_encode(struct kcn_buf *kb, const struct kcn_msg_response *kmr)
{

	kcn_msg_pkt_init(kb);
	kcn_buf_put8(kb, kmr->kmr_error);
	kcn_buf_put8(kb, kmr->kmr_score);
	if (kmr->kmr_loc != NULL)
		kcn_buf_put(kb, kmr->kmr_loc, kmr->kmr_loclen);
	kcn_msg_header_encode(kb, KCN_MSG_TYPE_RESPONSE);
}

bool
kcn_msg_response_decode(struct kcn_buf *kb, const struct kcn_msg_header *kmh,
    struct kcn_msg_response *kmr)
{
	size_t len = kmh->kmh_len;

	if (kcn_buf_trailingdata(kb) < KCN_MSG_RESPONSE_MINSIZ ||
	    len < KCN_MSG_RESPONSE_MINSIZ) {
		errno = EINVAL; /* XXX */
		goto bad;
	}
	kmr->kmr_error = kcn_buf_get8(kb);
	kmr->kmr_score = kcn_buf_get8(kb);
	kmr->kmr_loclen = len - KCN_MSG_RESPONSE_MINSIZ;
	if (kmr->kmr_loclen > 0)
		kmr->kmr_loc = kcn_buf_current(kb);
	else
		kmr->kmr_loc = NULL;
	kcn_buf_forward(kb, kmr->kmr_loclen);
	kcn_buf_trim_head(kb, kcn_buf_headingdata(kb));
	return true;
  bad:
	return false;
}

void
kcn_msg_add_encode(struct kcn_buf *kb, const struct kcn_msg_add *kma)
{

	kcn_msg_pkt_init(kb);
	kcn_buf_put8(kb, kma->kma_type);
	kcn_buf_put64(kb, kma->kma_time);
	kcn_buf_put64(kb, kma->kma_val);
	kcn_buf_put(kb, kma->kma_loc, kma->kma_loclen);
	kcn_msg_header_encode(kb, KCN_MSG_TYPE_ADD);
}

bool
kcn_msg_add_decode(struct kcn_buf *kb, const struct kcn_msg_header *kmh,
    struct kcn_msg_add *kma)
{

	if (kcn_buf_trailingdata(kb) < KCN_MSG_ADD_MINSIZ ||
	    kmh->kmh_len < KCN_MSG_ADD_MINSIZ) {
		errno = EINVAL;
		goto bad;
	}
	kma->kma_type = kcn_buf_get8(kb);
	kma->kma_time = kcn_buf_get64(kb);
	kma->kma_val = kcn_buf_get64(kb);
	kma->kma_loc = kcn_buf_current(kb);
	kma->kma_loclen = kmh->kmh_len - KCN_MSG_ADD_MINSIZ;
	kcn_buf_trim_head(kb, kmh->kmh_len);
	return true;
  bad:
	return false;
}
