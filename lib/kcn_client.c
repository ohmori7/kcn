#include <errno.h>
#include <stdbool.h>
#include <stddef.h>

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

static int
kcn_client_read(struct kcn_net *kn, struct kcn_pkt *kp, void *arg)
{
	struct kcn_msg_header kmh;
	struct kcn_msg_response *kmr = arg;

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

bool
kcn_client_search(struct kcn_info *ki, const struct kcn_msg_query *kmq)
{
	struct sockaddr_storage ss;
	struct kcn_msg_response kmr;
	struct event_base *evb;
	struct kcn_net *kn;
	struct kcn_pkt okp;
	char name[KCN_SOCKNAMELEN];
	int fd;

	evb = event_init();
	if (evb == NULL) {
		KCN_LOG(ERR, "cannot allocate event base");
		goto bad1;
	}

	if (! kcn_sockaddr_aton(&ss, KCN_NETSTAT_SERVER_STR_DEFAULT,
	    KCN_NETSTAT_PORT_STR_DEFAULT)) {
		KCN_LOG(ERR, "cannot get numeric server address");
		goto bad1;
	}

	fd = kcn_socket_connect(&ss);
	if (fd == -1)
		goto bad1;

	kcn_sockaddr_ntoa(name, sizeof(name), &ss);
	kcn_msg_response_init(&kmr, ki);
	kn = kcn_net_new(evb, fd, KCN_MSG_MAXSIZ, name, kcn_client_read, &kmr);
	if (kn == NULL) {
		KCN_LOG(ERR, "cannot allocate networking structure");
		goto bad2;
	}

	kcn_net_opkt(kn, &okp);
	kcn_msg_query_encode(&okp, kmq);

	if (! kcn_net_write(kn, &okp))
		goto bad2;
	if (! kcn_net_read_enable(kn))
		goto bad2;
	if (! kcn_net_loop(kn))
		goto bad2;
	if (kmr.kmr_error != 0) {
		errno = kmr.kmr_error;
		goto bad2;
	}
	if (kcn_info_nlocs(ki) == 0) {
		errno = ESRCH;
		goto bad2;
	}
	kcn_net_destroy(kn);
	event_base_free(evb);
	return true;
  bad2:
	if (kn != NULL)
		kcn_net_destroy(kn);
	else
		kcn_socket_close(&fd);
  bad1:
	if (evb != NULL)
		event_base_free(evb);
	return false;
}
