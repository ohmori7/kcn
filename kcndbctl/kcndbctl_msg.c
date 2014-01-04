#include <err.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event.h>

#include "kcn.h"
#include "kcn_log.h"
#include "kcn_str.h"
#include "kcn_buf.h"
#include "kcn_net.h"
#include "kcn_time.h"
#include "kcn_eq.h"
#include "kcn_msg.h"
#include "kcn_client.h"
#include "kcndbctl_msg.h"

int
kcndbctl_msg_add_send(enum kcn_eq_type type, struct kcn_net *kn,
    const char *v, const char *loc)
{
	struct kcn_msg_add kma;
	unsigned long long ullv;

	kma.kma_type = type;
	kma.kma_time = KCN_TIME_NOW;
	if (! kcn_strtoull(v, 0, ULLONG_MAX, &ullv))
		goto bad;
	kma.kma_val = ullv;
	kma.kma_loc = loc;
	kma.kma_loclen = strlen(loc);
	if (! kcn_client_add_send(kn, &kma)) {
		KCN_LOG(ERR, "cannot send add message");
		goto bad;
	}
	if (! kcn_net_loop(kn))
		goto bad;
	return EXIT_SUCCESS;
  bad:
	return EXIT_FAILURE;
}
