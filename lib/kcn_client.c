#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include <netinet/in.h>

#include <event.h>

#include "kcn.h"
#include "kcn_info.h"
#include "kcn_formula.h"
#include "kcn_log.h"
#include "kcn_socket.h"
#include "kcn_sockaddr.h"
#include "kcn_pkt.h"
#include "kcn_msg.h"
#include "kcn_net.h"
#include "kcn_netstat.h"
#include "kcn_client.h"

static int kcn_client_read(struct kcn_net *, struct kcn_pkt *, void *);

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
kcn_client_read(struct kcn_net *kn, struct kcn_pkt *kp, void *arg)
{
	struct kcn_msg_response *kmr = arg;
	struct kcn_msg_header kmh;

	(void)kn;
	if (! kcn_msg_header_decode(kp, &kmh))
		goto bad;
	if (kmh.kmh_type != KCN_MSG_TYPE_RESPONSE) {
		errno = EINVAL;
		goto bad;
	}
	if (! kcn_msg_response_decode(kp, &kmh, kmr))
		goto bad;
	return 0;
  bad:
	kmr->kmr_error = errno;
	return errno;
}

static bool
kcn_client_query_send(struct kcn_net *kn, const struct kcn_msg_query *kmq)
{
	struct kcn_pkt kp;

	kcn_net_opkt(kn, &kp);
	kcn_msg_query_encode(&kp, kmq);
	if (! kcn_net_write(kn, &kp))
		return false;
	return kcn_net_read_enable(kn);
}

bool
kcn_client_add_send(struct kcn_net *kn, const struct kcn_msg_add *kma)
{
	struct kcn_pkt kp;

	kcn_net_opkt(kn, &kp);
	kcn_msg_add_encode(&kp, kma);
	return kcn_net_write(kn, &kp);
}

bool
kcn_client_search(struct kcn_info *ki, const struct kcn_msg_query *kmq)
{
	struct event_base *evb;
	struct kcn_net *kn;
	struct kcn_msg_response kmr;

	evb = event_init();
	if (evb == NULL)
		goto bad;

	kn = kcn_client_init(evb, &kmr);
	if (kn == NULL)
		goto bad;

	kcn_msg_response_init(&kmr, ki);
	if (! kcn_client_query_send(kn, kmq))
		goto bad;
	if (! kcn_net_loop(kn))
		goto bad;
	if (kmr.kmr_error != 0) {
		errno = kmr.kmr_error;
		goto bad;
	}
	if (kcn_info_nlocs(ki) == 0) {
		errno = ESRCH;
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
