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

struct kcn_client {
	int kc_fd;
	struct event_base *kc_evb;
	struct kcn_net *kc_kn;
	struct kcn_msg_response kc_kmr;
};

static void kcn_client_destroy(struct kcn_client *);
static int kcn_client_read(struct kcn_net *, struct kcn_pkt *, void *);

static struct kcn_client *
kcn_client_new(void)
{
	struct kcn_client *kc;
	struct sockaddr_storage ss;
	char name[KCN_SOCKNAMELEN];

	kc = malloc(sizeof(*kc));
	if (kc == NULL)
		goto bad;
	kc->kc_fd = -1;
	kc->kc_evb = NULL;
	kc->kc_kn = NULL;

	if (! kcn_sockaddr_aton(&ss, KCN_NETSTAT_SERVER_STR_DEFAULT,
	    KCN_NETSTAT_PORT_STR_DEFAULT)) {
		KCN_LOG(ERR, "cannot get numeric server address");
		goto bad;
	}
	kc->kc_fd = kcn_socket_connect(&ss);
	if (kc->kc_fd == -1)
		goto bad;

	kc->kc_evb = event_init();
	if (kc->kc_evb == NULL) {
		KCN_LOG(ERR, "cannot allocate event base");
		goto bad;
	}

	kcn_sockaddr_ntoa(name, sizeof(name), &ss);
	kc->kc_kn = kcn_net_new(kc->kc_evb, kc->kc_fd, KCN_MSG_MAXSIZ, name,
	    kcn_client_read, kc);
	if (kc->kc_kn == NULL) {
		KCN_LOG(ERR, "cannot allocate networking structure");
		goto bad;
	}

	return kc;
  bad:
	kcn_client_destroy(kc);
	return NULL;
}

static void
kcn_client_destroy(struct kcn_client *kc)
{

	if (kc == NULL)
		return;
	if (kc->kc_evb != NULL)
		event_base_free(kc->kc_evb);
	if (kc->kc_kn != NULL)
		kcn_net_destroy(kc->kc_kn);
	else
		kcn_socket_close(&kc->kc_fd);
	free(kc);
}

static int
kcn_client_read(struct kcn_net *kn, struct kcn_pkt *kp, void *arg)
{
	struct kcn_client *kc = arg;
	struct kcn_msg_header kmh;

	(void)kn;
	if (! kcn_msg_header_decode(kp, &kmh))
		goto bad;
	if (kmh.kmh_type != KCN_MSG_TYPE_RESPONSE) {
		errno = EINVAL;
		goto bad;
	}
	if (! kcn_msg_response_decode(kp, &kmh, &kc->kc_kmr))
		goto bad;
	return 0;
  bad:
	kc->kc_kmr.kmr_error = errno;
	return errno;
}

static bool
kcn_client_query_send(struct kcn_client *kc, const struct kcn_msg_query *kmq)
{
	struct kcn_pkt kp;

	kcn_net_opkt(kc->kc_kn, &kp);
	kcn_msg_query_encode(&kp, kmq);
	if (! kcn_net_write(kc->kc_kn, &kp))
		return false;
	return kcn_net_read_enable(kc->kc_kn);
}

static bool
kcn_client_loop(struct kcn_client *kc)
{

	return kcn_net_loop(kc->kc_kn);
}

bool
kcn_client_search(struct kcn_info *ki, const struct kcn_msg_query *kmq)
{
	struct kcn_client *kc;

	kc = kcn_client_new();
	if (kc == NULL)
		goto bad;

	kcn_msg_response_init(&kc->kc_kmr, ki);
	if (! kcn_client_query_send(kc, kmq))
		goto bad;
	if (! kcn_client_loop(kc))
		goto bad;
	if (kc->kc_kmr.kmr_error != 0) {
		errno = kc->kc_kmr.kmr_error;
		goto bad;
	}
	if (kcn_info_nlocs(ki) == 0) {
		errno = ESRCH;
		goto bad;
	}
	kcn_client_destroy(kc);
	return true;
  bad:
	kcn_client_destroy(kc);
	return false;
}
