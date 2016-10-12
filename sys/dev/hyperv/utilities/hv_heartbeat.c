/*-
 * Copyright (c) 2014,2016 Microsoft Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice unmodified, this list of conditions, and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $FreeBSD$
 */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/timetc.h>
#include <sys/syscallsubr.h>

#include <dev/hyperv/include/hyperv.h>
#include <dev/hyperv/include/vmbus.h>
#include <dev/hyperv/utilities/hv_utilreg.h>
#include "hv_util.h"
#include "vmbus_if.h"

/* Heartbeat Service */
static const struct hyperv_guid service_guid = { .hv_guid =
	{0x39, 0x4f, 0x16, 0x57, 0x15, 0x91, 0x78, 0x4e,
	0xab, 0x55, 0x38, 0x2f, 0x3b, 0xd5, 0x42, 0x2d} };

/**
 * Process heartbeat message
 */
static void
hv_heartbeat_cb(struct vmbus_channel *channel, void *context)
{
	uint8_t*		buf;
	int			recvlen;
	uint64_t		requestid;
	int			ret;

	struct hv_vmbus_heartbeat_msg_data*	heartbeat_msg;
	struct hv_vmbus_icmsg_hdr*		icmsghdrp;
	hv_util_sc			*softc;

	softc = (hv_util_sc*)context;
	buf = softc->receive_buffer;;

	recvlen = PAGE_SIZE;
	ret = vmbus_chan_recv(channel, buf, &recvlen, &requestid);
	KASSERT(ret != ENOBUFS, ("hvheartbeat recvbuf is not large enough"));
	/* XXX check recvlen to make sure that it contains enough data */

	if ((ret == 0) && recvlen > 0) {

	    icmsghdrp = (struct hv_vmbus_icmsg_hdr *)
		&buf[sizeof(struct hv_vmbus_pipe_hdr)];

	    if (icmsghdrp->icmsgtype == HV_ICMSGTYPE_NEGOTIATE) {
		hv_negotiate_version(icmsghdrp, NULL, buf);

	    } else {
		heartbeat_msg =
		    (struct hv_vmbus_heartbeat_msg_data *)
			&buf[sizeof(struct hv_vmbus_pipe_hdr) +
			     sizeof(struct hv_vmbus_icmsg_hdr)];

		heartbeat_msg->seq_num += 1;
	    }

	    icmsghdrp->icflags = HV_ICMSGHDRFLAG_TRANSACTION |
				 HV_ICMSGHDRFLAG_RESPONSE;

	    vmbus_chan_send(channel, VMBUS_CHANPKT_TYPE_INBAND, 0,
	        buf, recvlen, requestid);
	}
}

static int
hv_heartbeat_probe(device_t dev)
{
	if (resource_disabled("hvheartbeat", 0))
		return ENXIO;

	if (VMBUS_PROBE_GUID(device_get_parent(dev), dev, &service_guid) == 0) {
		device_set_desc(dev, "Hyper-V Heartbeat Service");
		return BUS_PROBE_DEFAULT;
	}
	return ENXIO;
}

static int
hv_heartbeat_attach(device_t dev)
{
	hv_util_sc *softc = (hv_util_sc*)device_get_softc(dev);

	softc->callback = hv_heartbeat_cb;

	return hv_util_attach(dev);
}

static device_method_t heartbeat_methods[] = {
	/* Device interface */
	DEVMETHOD(device_probe, hv_heartbeat_probe),
	DEVMETHOD(device_attach, hv_heartbeat_attach),
	DEVMETHOD(device_detach, hv_util_detach),
	{ 0, 0 }
};

static driver_t heartbeat_driver = { "hvheartbeat", heartbeat_methods, sizeof(hv_util_sc)};

static devclass_t heartbeat_devclass;

DRIVER_MODULE(hv_heartbeat, vmbus, heartbeat_driver, heartbeat_devclass, NULL, NULL);
MODULE_VERSION(hv_heartbeat, 1);
MODULE_DEPEND(hv_heartbeat, vmbus, 1, 1, 1);
