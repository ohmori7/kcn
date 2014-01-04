#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_eq.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcn_buf.h"
#include "kcn_msg.h"
#include "kcn_net.h"
#include "kcn_netstat.h"
#include "kcn_client.h"

struct kcn_client_response {
	int kcr_error;
	struct kcn_info *kcr_ki;
};

static int kcn_client_read(struct kcn_net *, struct kcn_buf *, void *);

struct kcn_net *
kcn_client_init(struct event_base *evb, void *data)
{
	struct sockaddr_storage ss;
	char name[KCN_SOCKNAMELEN];
	struct kcn_net *kn;
	int fd;

	if (! kcn_sockaddr_aton(&ss, KCN_NETSTAT_SERVER_STR_DEFAULT,
	    KCN_NETSTAT_PORT_STR_DEFAULT)) {
		KCN_LOG(ERR, "cannot get numeric server address");
		goto bad;
	}
	fd = kcn_socket_connect(&ss);
	if (fd == -1)
		goto bad;

	kcn_sockaddr_ntoa(name, sizeof(name), &ss);
	kn = kcn_net_new(evb, fd, KCN_MSG_MAXSIZ, name, kcn_client_read, data);
	if (kn == NULL) {
		KCN_LOG(ERR, "cannot allocate networking structure");
		goto bad;
	}
	return kn;
  bad:
	return NULL;
}

void
kcn_client_finish(struct kcn_net *kn)
{

	kcn_net_destroy(kn);
}

static int
kcn_client_read(struct kcn_net *kn, struct kcn_buf *kb, void *arg)
{
	struct kcn_client_response *kcr = arg;
	struct kcn_info *ki = kcr->kcr_ki;
	struct kcn_msg_response kmr;
	struct kcn_msg_header kmh;

	(void)kn;
	for (;;) {
		if (! kcn_msg_header_decode(kb, &kmh))
			break;
		if (kmh.kmh_type != KCN_MSG_TYPE_RESPONSE) {
			errno = EINVAL;
			break;
		}
		kcn_msg_response_init(&kmr);
		if (! kcn_msg_response_decode(kb, &kmh, &kmr))
			break;
		if (kmr.kmr_loclen == 0) {
			if (kmr.kmr_error != EAGAIN)
				errno = kmr.kmr_error;
			break;
		}
		if (kcn_info_maxnlocs(ki) == kcn_info_nlocs(ki)) {
			errno = ETOOMANYREFS; /* XXX */
			break;
		}
		if (! kcn_info_loc_add(ki,
		    kmr.kmr_loc, kmr.kmr_loclen, kmr.kmr_score))
			break;
	}
	if (errno != EAGAIN)
		kcr->kcr_error = errno;
	return kcr->kcr_error;
}

static bool
kcn_client_query_send(struct kcn_net *kn, const struct kcn_msg_query *kmq)
{
	struct kcn_buf kb;

	kcn_net_opkt(kn, &kb);
	kcn_msg_query_encode(&kb, kmq);
	if (! kcn_net_write(kn, &kb))
		return false;
	return kcn_net_read_enable(kn);
}

bool
kcn_client_add_send(struct kcn_net *kn, const struct kcn_msg_add *kma)
{
	struct kcn_buf kb;

	kcn_net_opkt(kn, &kb);
	kcn_msg_add_encode(&kb, kma);
	return kcn_net_write(kn, &kb);
}

bool
kcn_client_search(struct kcn_info *ki, const struct kcn_msg_query *kmq)
{
	struct event_base *evb;
	struct kcn_net *kn;
	struct kcn_client_response kcr;

	evb = event_init();
	if (evb == NULL)
		goto bad;

	kcr.kcr_error = 0;
	kcr.kcr_ki = ki;
	kn = kcn_client_init(evb, &kcr);
	if (kn == NULL)
		goto bad;

	if (! kcn_client_query_send(kn, kmq))
		goto bad;
	if (! kcn_net_loop(kn))
		goto bad;
	if (kcr.kcr_error != 0) {
		errno = kcr.kcr_error;
		goto bad;
	}
	kcn_client_finish(kn);
	event_base_free(evb);
	return true;
  bad:
	if (evb != NULL) {
		kcn_net_destroy(kn);
		event_base_free(evb);
	}
	return false;
}
