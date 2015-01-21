/*-
 * Copyright 2003 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <anholt@FreeBSD.org>
 *
 */

#include <sys/cdefs.h>
__FBSDID("$FreeBSD$");

/** @file drm_irq.c
 * Support code for handling setup/teardown of interrupt handlers and
 * handing interrupt handlers off to the drivers.
 */

#include <dev/drm2/drmP.h>
#include <dev/drm2/drm.h>

MALLOC_DEFINE(DRM_MEM_VBLANK, "drm_vblank", "DRM VBLANK Handling Data");

/* Access macro for slots in vblank timestamp ringbuffer. */
#define vblanktimestamp(dev, crtc, count) ( \
	(dev)->_vblank_time[(crtc) * DRM_VBLANKTIME_RBSIZE + \
	((count) % DRM_VBLANKTIME_RBSIZE)])

/* Retry timestamp calculation up to 3 times to satisfy
 * drm_timestamp_precision before giving up.
 */
#define DRM_TIMESTAMP_MAXRETRIES 3

/* Threshold in nanoseconds for detection of redundant
 * vblank irq in drm_handle_vblank(). 1 msec should be ok.
 */
#define DRM_REDUNDANT_VBLIRQ_THRESH_NS 1000000

int drm_irq_by_busid(struct drm_device *dev, void *data,
		     struct drm_file *file_priv)
{
	struct drm_irq_busid *irq = data;

	if ((irq->busnum >> 8) != dev->pci_domain ||
	    (irq->busnum & 0xff) != dev->pci_bus ||
	    irq->devnum != dev->pci_slot ||
	    irq->funcnum != dev->pci_func)
		return EINVAL;

	irq->irq = dev->irq;

	DRM_DEBUG("%d:%d:%d => IRQ %d\n",
	    irq->busnum, irq->devnum, irq->funcnum, irq->irq);

	return 0;
}

static void
drm_irq_handler_wrap(void *arg)
{
	struct drm_device *dev = arg;

	mtx_lock(&dev->irq_lock);
	dev->driver->irq_handler(arg);
	mtx_unlock(&dev->irq_lock);
}

int
drm_irq_install(struct drm_device *dev)
{
	int retcode;

	if (dev->irq == 0 || dev->dev_private == NULL)
		return (EINVAL);

	DRM_DEBUG("irq=%d\n", dev->irq);

	DRM_LOCK(dev);
	if (dev->irq_enabled) {
		DRM_UNLOCK(dev);
		return EBUSY;
	}
	dev->irq_enabled = 1;

	dev->context_flag = 0;

	/* Before installing handler */
	if (dev->driver->irq_preinstall)
		dev->driver->irq_preinstall(dev);
	DRM_UNLOCK(dev);

	/* Install handler */
	retcode = bus_setup_intr(dev->device, dev->irqr,
	    INTR_TYPE_TTY | INTR_MPSAFE, NULL,
	    (dev->driver->driver_features & DRIVER_LOCKLESS_IRQ) != 0 ?
		drm_irq_handler_wrap : dev->driver->irq_handler,
	    dev, &dev->irqh);
	if (retcode != 0)
		goto err;

	/* After installing handler */
	DRM_LOCK(dev);
	if (dev->driver->irq_postinstall)
		dev->driver->irq_postinstall(dev);
	DRM_UNLOCK(dev);

	return (0);
err:
	device_printf(dev->device, "Error setting interrupt: %d\n", retcode);
	dev->irq_enabled = 0;

	return (retcode);
}

int drm_irq_uninstall(struct drm_device *dev)
{
	int i;

	if (!dev->irq_enabled)
		return EINVAL;

	dev->irq_enabled = 0;

	/*
	* Wake up any waiters so they don't hang.
	*/
	if (dev->num_crtcs) {
		mtx_lock(&dev->vbl_lock);
		for (i = 0; i < dev->num_crtcs; i++) {
			wakeup(&dev->_vblank_count[i]);
			dev->vblank_enabled[i] = 0;
			dev->last_vblank[i] =
				dev->driver->get_vblank_counter(dev, i);
		}
		mtx_unlock(&dev->vbl_lock);
	}

	DRM_DEBUG("irq=%d\n", dev->irq);

	if (dev->driver->irq_uninstall)
		dev->driver->irq_uninstall(dev);

	DRM_UNLOCK(dev);
	bus_teardown_intr(dev->device, dev->irqr, dev->irqh);
	DRM_LOCK(dev);

	return 0;
}

int drm_control(struct drm_device *dev, void *data, struct drm_file *file_priv)
{
	struct drm_control *ctl = data;
	int err;

	switch (ctl->func) {
	case DRM_INST_HANDLER:
		/* Handle drivers whose DRM used to require IRQ setup but the
		 * no longer does.
		 */
		if (!drm_core_check_feature(dev, DRIVER_HAVE_IRQ))
			return 0;
		if (drm_core_check_feature(dev, DRIVER_MODESET))
			return 0;
		if (dev->if_version < DRM_IF_VERSION(1, 2) &&
		    ctl->irq != dev->irq)
			return EINVAL;
		return drm_irq_install(dev);
	case DRM_UNINST_HANDLER:
		if (!drm_core_check_feature(dev, DRIVER_HAVE_IRQ))
			return 0;
		if (drm_core_check_feature(dev, DRIVER_MODESET))
			return 0;
		DRM_LOCK(dev);
		err = drm_irq_uninstall(dev);
		DRM_UNLOCK(dev);
		return err;
	default:
		return EINVAL;
	}
}

#define NSEC_PER_USEC	1000L
#define NSEC_PER_SEC	1000000000L

int64_t
timeval_to_ns(const struct timeval *tv)
{
	return ((int64_t)tv->tv_sec * NSEC_PER_SEC) +
		tv->tv_usec * NSEC_PER_USEC;
}

struct timeval
ns_to_timeval(const int64_t nsec)
{
        struct timeval tv;
	long rem;

	if (nsec == 0) {
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		return (tv);
	}

        tv.tv_sec = nsec / NSEC_PER_SEC;
	rem = nsec % NSEC_PER_SEC;
        if (rem < 0) {
                tv.tv_sec--;
                rem += NSEC_PER_SEC;
        }
	tv.tv_usec = rem / 1000;
        return (tv);
}

/*
 * Clear vblank timestamp buffer for a crtc.
 */
static void clear_vblank_timestamps(struct drm_device *dev, int crtc)
{
	memset(&dev->_vblank_time[crtc * DRM_VBLANKTIME_RBSIZE], 0,
		DRM_VBLANKTIME_RBSIZE * sizeof(struct timeval));
}

static int64_t
abs64(int64_t x)
{

	return (x < 0 ? -x : x);
}

/*
 * Disable vblank irq's on crtc, make sure that last vblank count
 * of hardware and corresponding consistent software vblank counter
 * are preserved, even if there are any spurious vblank irq's after
 * disable.
 */
static void vblank_disable_and_save(struct drm_device *dev, int crtc)
{
	u32 vblcount;
	int64_t diff_ns;
	int vblrc;
	struct timeval tvblank;

	/* Prevent vblank irq processing while disabling vblank irqs,
	 * so no updates of timestamps or count can happen after we've
	 * disabled. Needed to prevent races in case of delayed irq's.
	 */
	mtx_lock(&dev->vblank_time_lock);

	dev->driver->disable_vblank(dev, crtc);
	dev->vblank_enabled[crtc] = 0;

	/* No further vblank irq's will be processed after
	 * this point. Get current hardware vblank count and
	 * vblank timestamp, repeat until they are consistent.
	 *
	 * FIXME: There is still a race condition here and in
	 * drm_update_vblank_count() which can cause off-by-one
	 * reinitialization of software vblank counter. If gpu
	 * vblank counter doesn't increment exactly at the leading
	 * edge of a vblank interval, then we can lose 1 count if
	 * we happen to execute between start of vblank and the
	 * delayed gpu counter increment.
	 */
	do {
		dev->last_vblank[crtc] = dev->driver->get_vblank_counter(dev, crtc);
		vblrc = drm_get_last_vbltimestamp(dev, crtc, &tvblank, 0);
	} while (dev->last_vblank[crtc] != dev->driver->get_vblank_counter(dev, crtc));

	/* Compute time difference to stored timestamp of last vblank
	 * as updated by last invocation of drm_handle_vblank() in vblank irq.
	 */
	vblcount = atomic_read(&dev->_vblank_count[crtc]);
	diff_ns = timeval_to_ns(&tvblank) -
		  timeval_to_ns(&vblanktimestamp(dev, crtc, vblcount));

	/* If there is at least 1 msec difference between the last stored
	 * timestamp and tvblank, then we are currently executing our
	 * disable inside a new vblank interval, the tvblank timestamp
	 * corresponds to this new vblank interval and the irq handler
	 * for this vblank didn't run yet and won't run due to our disable.
	 * Therefore we need to do the job of drm_handle_vblank() and
	 * increment the vblank counter by one to account for this vblank.
	 *
	 * Skip this step if there isn't any high precision timestamp
	 * available. In that case we can't account for this and just
	 * hope for the best.
	 */
	if ((vblrc > 0) && (abs64(diff_ns) > 1000000)) {
		atomic_inc(&dev->_vblank_count[crtc]);
	}

	/* Invalidate all timestamps while vblank irq's are off. */
	clear_vblank_timestamps(dev, crtc);

	mtx_unlock(&dev->vblank_time_lock);
}

static void vblank_disable_fn(void * arg)
{
	struct drm_device *dev = (struct drm_device *)arg;
	int i;

	if (!dev->vblank_disable_allowed)
		return;

	for (i = 0; i < dev->num_crtcs; i++) {
		mtx_lock(&dev->vbl_lock);
		if (atomic_read(&dev->vblank_refcount[i]) == 0 &&
		    dev->vblank_enabled[i]) {
			DRM_DEBUG("disabling vblank on crtc %d\n", i);
			vblank_disable_and_save(dev, i);
		}
		mtx_unlock(&dev->vbl_lock);
	}
}

void drm_vblank_cleanup(struct drm_device *dev)
{
	/* Bail if the driver didn't call drm_vblank_init() */
	if (dev->num_crtcs == 0)
		return;

	callout_stop(&dev->vblank_disable_callout);

	vblank_disable_fn(dev);

	free(dev->_vblank_count, DRM_MEM_VBLANK);
	free(dev->vblank_refcount, DRM_MEM_VBLANK);
	free(dev->vblank_enabled, DRM_MEM_VBLANK);
	free(dev->last_vblank, DRM_MEM_VBLANK);
	free(dev->last_vblank_wait, DRM_MEM_VBLANK);
	free(dev->vblank_inmodeset, DRM_MEM_VBLANK);
	free(dev->_vblank_time, DRM_MEM_VBLANK);

	dev->num_crtcs = 0;
}

int drm_vblank_init(struct drm_device *dev, int num_crtcs)
{
	int i;

	callout_init(&dev->vblank_disable_callout, CALLOUT_MPSAFE);
#if 0
	mtx_init(&dev->vbl_lock, "drmvbl", NULL, MTX_DEF);
#endif
	mtx_init(&dev->vblank_time_lock, "drmvtl", NULL, MTX_DEF);

	dev->num_crtcs = num_crtcs;

	dev->_vblank_count = malloc(sizeof(atomic_t) * num_crtcs,
	    DRM_MEM_VBLANK, M_WAITOK);
	dev->vblank_refcount = malloc(sizeof(atomic_t) * num_crtcs,
	    DRM_MEM_VBLANK, M_WAITOK);
	dev->vblank_enabled = malloc(num_crtcs * sizeof(int),
	    DRM_MEM_VBLANK, M_WAITOK | M_ZERO);
	dev->last_vblank = malloc(num_crtcs * sizeof(u32),
	    DRM_MEM_VBLANK, M_WAITOK | M_ZERO);
	dev->last_vblank_wait = malloc(num_crtcs * sizeof(u32),
	    DRM_MEM_VBLANK, M_WAITOK | M_ZERO);
	dev->vblank_inmodeset = malloc(num_crtcs * sizeof(int),
	    DRM_MEM_VBLANK, M_WAITOK | M_ZERO);
	dev->_vblank_time = malloc(num_crtcs * DRM_VBLANKTIME_RBSIZE *
	    sizeof(struct timeval), DRM_MEM_VBLANK, M_WAITOK | M_ZERO);
	DRM_INFO("Supports vblank timestamp caching Rev 1 (10.10.2010).\n");

	/* Driver specific high-precision vblank timestamping supported? */
	if (dev->driver->get_vblank_timestamp)
		DRM_INFO("Driver supports precise vblank timestamp query.\n");
	else
		DRM_INFO("No driver support for vblank timestamp query.\n");

	/* Zero per-crtc vblank stuff */
	for (i = 0; i < num_crtcs; i++) {
		atomic_set(&dev->_vblank_count[i], 0);
		atomic_set(&dev->vblank_refcount[i], 0);
	}

	dev->vblank_disable_allowed = 0;
	return 0;
}

void
drm_calc_timestamping_constants(struct drm_crtc *crtc)
{
	int64_t linedur_ns = 0, pixeldur_ns = 0, framedur_ns = 0;
	uint64_t dotclock;

	/* Dot clock in Hz: */
	dotclock = (uint64_t) crtc->hwmode.clock * 1000;

	/* Fields of interlaced scanout modes are only halve a frame duration.
	 * Double the dotclock to get halve the frame-/line-/pixelduration.
	 */
	if (crtc->hwmode.flags & DRM_MODE_FLAG_INTERLACE)
		dotclock *= 2;

	/* Valid dotclock? */
	if (dotclock > 0) {
		/* Convert scanline length in pixels and video dot clock to
		 * line duration, frame duration and pixel duration in
		 * nanoseconds:
		 */
		pixeldur_ns = (int64_t)1000000000 / dotclock;
		linedur_ns  = ((uint64_t)crtc->hwmode.crtc_htotal *
		    1000000000) / dotclock;
		framedur_ns = (int64_t)crtc->hwmode.crtc_vtotal * linedur_ns;
	} else
		DRM_ERROR("crtc %d: Can't calculate constants, dotclock = 0!\n",
			  crtc->base.id);

	crtc->pixeldur_ns = pixeldur_ns;
	crtc->linedur_ns  = linedur_ns;
	crtc->framedur_ns = framedur_ns;

	DRM_DEBUG("crtc %d: hwmode: htotal %d, vtotal %d, vdisplay %d\n",
		  crtc->base.id, crtc->hwmode.crtc_htotal,
		  crtc->hwmode.crtc_vtotal, crtc->hwmode.crtc_vdisplay);
	DRM_DEBUG("crtc %d: clock %d kHz framedur %d linedur %d, pixeldur %d\n",
		  crtc->base.id, (int) dotclock/1000, (int) framedur_ns,
		  (int) linedur_ns, (int) pixeldur_ns);
}

/**
 * drm_calc_vbltimestamp_from_scanoutpos - helper routine for kms
 * drivers. Implements calculation of exact vblank timestamps from
 * given drm_display_mode timings and current video scanout position
 * of a crtc. This can be called from within get_vblank_timestamp()
 * implementation of a kms driver to implement the actual timestamping.
 *
 * Should return timestamps conforming to the OML_sync_control OpenML
 * extension specification. The timestamp corresponds to the end of
 * the vblank interval, aka start of scanout of topmost-leftmost display
 * pixel in the following video frame.
 *
 * Requires support for optional dev->driver->get_scanout_position()
 * in kms driver, plus a bit of setup code to provide a drm_display_mode
 * that corresponds to the true scanout timing.
 *
 * The current implementation only handles standard video modes. It
 * returns as no operation if a doublescan or interlaced video mode is
 * active. Higher level code is expected to handle this.
 *
 * @dev: DRM device.
 * @crtc: Which crtc's vblank timestamp to retrieve.
 * @max_error: Desired maximum allowable error in timestamps (nanosecs).
 *             On return contains true maximum error of timestamp.
 * @vblank_time: Pointer to struct timeval which should receive the timestamp.
 * @flags: Flags to pass to driver:
 *         0 = Default.
 *         DRM_CALLED_FROM_VBLIRQ = If function is called from vbl irq handler.
 * @refcrtc: drm_crtc* of crtc which defines scanout timing.
 *
 * Returns negative value on error, failure or if not supported in current
 * video mode:
 *
 * -EINVAL   - Invalid crtc.
 * -EAGAIN   - Temporary unavailable, e.g., called before initial modeset.
 * -ENOTSUPP - Function not supported in current display mode.
 * -EIO      - Failed, e.g., due to failed scanout position query.
 *
 * Returns or'ed positive status flags on success:
 *
 * DRM_VBLANKTIME_SCANOUTPOS_METHOD - Signal this method used for timestamping.
 * DRM_VBLANKTIME_INVBL - Timestamp taken while scanout was in vblank interval.
 *
 */
int
drm_calc_vbltimestamp_from_scanoutpos(struct drm_device *dev, int crtc,
    int *max_error, struct timeval *vblank_time, unsigned flags,
    struct drm_crtc *refcrtc)
{
	struct timeval stime, raw_time;
	struct drm_display_mode *mode;
	int vbl_status, vtotal, vdisplay;
	int vpos, hpos, i;
	int64_t framedur_ns, linedur_ns, pixeldur_ns, delta_ns, duration_ns;
	bool invbl;

	if (crtc < 0 || crtc >= dev->num_crtcs) {
		DRM_ERROR("Invalid crtc %d\n", crtc);
		return -EINVAL;
	}

	/* Scanout position query not supported? Should not happen. */
	if (!dev->driver->get_scanout_position) {
		DRM_ERROR("Called from driver w/o get_scanout_position()!?\n");
		return -EIO;
	}

	mode = &refcrtc->hwmode;
	vtotal = mode->crtc_vtotal;
	vdisplay = mode->crtc_vdisplay;

	/* Durations of frames, lines, pixels in nanoseconds. */
	framedur_ns = refcrtc->framedur_ns;
	linedur_ns  = refcrtc->linedur_ns;
	pixeldur_ns = refcrtc->pixeldur_ns;

	/* If mode timing undefined, just return as no-op:
	 * Happens during initial modesetting of a crtc.
	 */
	if (vtotal <= 0 || vdisplay <= 0 || framedur_ns == 0) {
		DRM_DEBUG("crtc %d: Noop due to uninitialized mode.\n", crtc);
		return -EAGAIN;
	}

	/* Get current scanout position with system timestamp.
	 * Repeat query up to DRM_TIMESTAMP_MAXRETRIES times
	 * if single query takes longer than max_error nanoseconds.
	 *
	 * This guarantees a tight bound on maximum error if
	 * code gets preempted or delayed for some reason.
	 */
	for (i = 0; i < DRM_TIMESTAMP_MAXRETRIES; i++) {
		/* Disable preemption to make it very likely to
		 * succeed in the first iteration.
		 */
		critical_enter();

		/* Get system timestamp before query. */
		getmicrouptime(&stime);

		/* Get vertical and horizontal scanout pos. vpos, hpos. */
		vbl_status = dev->driver->get_scanout_position(dev, crtc, &vpos, &hpos);

		/* Get system timestamp after query. */
		getmicrouptime(&raw_time);

		critical_exit();

		/* Return as no-op if scanout query unsupported or failed. */
		if (!(vbl_status & DRM_SCANOUTPOS_VALID)) {
			DRM_DEBUG("crtc %d : scanoutpos query failed [%d].\n",
				  crtc, vbl_status);
			return -EIO;
		}

		duration_ns = timeval_to_ns(&raw_time) - timeval_to_ns(&stime);

		/* Accept result with <  max_error nsecs timing uncertainty. */
		if (duration_ns <= (int64_t) *max_error)
			break;
	}

	/* Noisy system timing? */
	if (i == DRM_TIMESTAMP_MAXRETRIES) {
		DRM_DEBUG("crtc %d: Noisy timestamp %d us > %d us [%d reps].\n",
			  crtc, (int) duration_ns/1000, *max_error/1000, i);
	}

	/* Return upper bound of timestamp precision error. */
	*max_error = (int) duration_ns;

	/* Check if in vblank area:
	 * vpos is >=0 in video scanout area, but negative
	 * within vblank area, counting down the number of lines until
	 * start of scanout.
	 */
	invbl = vbl_status & DRM_SCANOUTPOS_INVBL;

	/* Convert scanout position into elapsed time at raw_time query
	 * since start of scanout at first display scanline. delta_ns
	 * can be negative if start of scanout hasn't happened yet.
	 */
	delta_ns = (int64_t)vpos * linedur_ns + (int64_t)hpos * pixeldur_ns;

	/* Is vpos outside nominal vblank area, but less than
	 * 1/100 of a frame height away from start of vblank?
	 * If so, assume this isn't a massively delayed vblank
	 * interrupt, but a vblank interrupt that fired a few
	 * microseconds before true start of vblank. Compensate
	 * by adding a full frame duration to the final timestamp.
	 * Happens, e.g., on ATI R500, R600.
	 *
	 * We only do this if DRM_CALLED_FROM_VBLIRQ.
	 */
	if ((flags & DRM_CALLED_FROM_VBLIRQ) && !invbl &&
	    ((vdisplay - vpos) < vtotal / 100)) {
		delta_ns = delta_ns - framedur_ns;

		/* Signal this correction as "applied". */
		vbl_status |= 0x8;
	}

	/* Subtract time delta from raw timestamp to get final
	 * vblank_time timestamp for end of vblank.
	 */
	*vblank_time = ns_to_timeval(timeval_to_ns(&raw_time) - delta_ns);

	DRM_DEBUG("crtc %d : v %d p(%d,%d)@ %jd.%jd -> %jd.%jd [e %d us, %d rep]\n",
		  crtc, (int)vbl_status, hpos, vpos, (uintmax_t)raw_time.tv_sec,
		  (uintmax_t)raw_time.tv_usec, (uintmax_t)vblank_time->tv_sec,
		  (uintmax_t)vblank_time->tv_usec, (int)duration_ns/1000, i);

	vbl_status = DRM_VBLANKTIME_SCANOUTPOS_METHOD;
	if (invbl)
		vbl_status |= DRM_VBLANKTIME_INVBL;

	return vbl_status;
}

/**
 * drm_get_last_vbltimestamp - retrieve raw timestamp for the most recent
 * vblank interval.
 *
 * @dev: DRM device
 * @crtc: which crtc's vblank timestamp to retrieve
 * @tvblank: Pointer to target struct timeval which should receive the timestamp
 * @flags: Flags to pass to driver:
 *         0 = Default.
 *         DRM_CALLED_FROM_VBLIRQ = If function is called from vbl irq handler.
 *
 * Fetches the system timestamp corresponding to the time of the most recent
 * vblank interval on specified crtc. May call into kms-driver to
 * compute the timestamp with a high-precision GPU specific method.
 *
 * Returns zero if timestamp originates from uncorrected do_gettimeofday()
 * call, i.e., it isn't very precisely locked to the true vblank.
 *
 * Returns non-zero if timestamp is considered to be very precise.
 */
u32 drm_get_last_vbltimestamp(struct drm_device *dev, int crtc,
			      struct timeval *tvblank, unsigned flags)
{
	int ret;

	/* Define requested maximum error on timestamps (nanoseconds). */
	int max_error = (int) drm_timestamp_precision * 1000;

	/* Query driver if possible and precision timestamping enabled. */
	if (dev->driver->get_vblank_timestamp && (max_error > 0)) {
		ret = dev->driver->get_vblank_timestamp(dev, crtc, &max_error,
							tvblank, flags);
		if (ret > 0)
			return (u32) ret;
	}

	/* GPU high precision timestamp query unsupported or failed.
	 * Return gettimeofday timestamp as best estimate.
	 */
	microtime(tvblank);

	return 0;
}

/**
 * drm_vblank_count - retrieve "cooked" vblank counter value
 * @dev: DRM device
 * @crtc: which counter to retrieve
 *
 * Fetches the "cooked" vblank count value that represents the number of
 * vblank events since the system was booted, including lost events due to
 * modesetting activity.
 */
u32 drm_vblank_count(struct drm_device *dev, int crtc)
{
	return atomic_read(&dev->_vblank_count[crtc]);
}

/**
 * drm_vblank_count_and_time - retrieve "cooked" vblank counter value
 * and the system timestamp corresponding to that vblank counter value.
 *
 * @dev: DRM device
 * @crtc: which counter to retrieve
 * @vblanktime: Pointer to struct timeval to receive the vblank timestamp.
 *
 * Fetches the "cooked" vblank count value that represents the number of
 * vblank events since the system was booted, including lost events due to
 * modesetting activity. Returns corresponding system timestamp of the time
 * of the vblank interval that corresponds to the current value vblank counter
 * value.
 */
u32 drm_vblank_count_and_time(struct drm_device *dev, int crtc,
			      struct timeval *vblanktime)
{
	u32 cur_vblank;

	/* Read timestamp from slot of _vblank_time ringbuffer
	 * that corresponds to current vblank count. Retry if
	 * count has incremented during readout. This works like
	 * a seqlock.
	 */
	do {
		cur_vblank = atomic_read(&dev->_vblank_count[crtc]);
		*vblanktime = vblanktimestamp(dev, crtc, cur_vblank);
		rmb();
	} while (cur_vblank != atomic_read(&dev->_vblank_count[crtc]));

	return cur_vblank;
}

/**
 * drm_update_vblank_count - update the master vblank counter
 * @dev: DRM device
 * @crtc: counter to update
 *
 * Call back into the driver to update the appropriate vblank counter
 * (specified by @crtc).  Deal with wraparound, if it occurred, and
 * update the last read value so we can deal with wraparound on the next
 * call if necessary.
 *
 * Only necessary when going from off->on, to account for frames we
 * didn't get an interrupt for.
 *
 * Note: caller must hold dev->vbl_lock since this reads & writes
 * device vblank fields.
 */
static void drm_update_vblank_count(struct drm_device *dev, int crtc)
{
	u32 cur_vblank, diff, tslot, rc;
	struct timeval t_vblank;

	/*
	 * Interrupts were disabled prior to this call, so deal with counter
	 * wrap if needed.
	 * NOTE!  It's possible we lost a full dev->max_vblank_count events
	 * here if the register is small or we had vblank interrupts off for
	 * a long time.
	 *
	 * We repeat the hardware vblank counter & timestamp query until
	 * we get consistent results. This to prevent races between gpu
	 * updating its hardware counter while we are retrieving the
	 * corresponding vblank timestamp.
	 */
	do {
		cur_vblank = dev->driver->get_vblank_counter(dev, crtc);
		rc = drm_get_last_vbltimestamp(dev, crtc, &t_vblank, 0);
	} while (cur_vblank != dev->driver->get_vblank_counter(dev, crtc));

	/* Deal with counter wrap */
	diff = cur_vblank - dev->last_vblank[crtc];
	if (cur_vblank < dev->last_vblank[crtc]) {
		diff += dev->max_vblank_count;

		DRM_DEBUG("last_vblank[%d]=0x%x, cur_vblank=0x%x => diff=0x%x\n",
			  crtc, dev->last_vblank[crtc], cur_vblank, diff);
	}

	DRM_DEBUG("enabling vblank interrupts on crtc %d, missed %d\n",
		  crtc, diff);

	/* Reinitialize corresponding vblank timestamp if high-precision query
	 * available. Skip this step if query unsupported or failed. Will
	 * reinitialize delayed at next vblank interrupt in that case.
	 */
	if (rc) {
		tslot = atomic_read(&dev->_vblank_count[crtc]) + diff;
		vblanktimestamp(dev, crtc, tslot) = t_vblank;
	}

	atomic_add(diff, &dev->_vblank_count[crtc]);
}

/**
 * drm_vblank_get - get a reference count on vblank events
 * @dev: DRM device
 * @crtc: which CRTC to own
 *
 * Acquire a reference count on vblank events to avoid having them disabled
 * while in use.
 *
 * RETURNS
 * Zero on success, nonzero on failure.
 */
int drm_vblank_get(struct drm_device *dev, int crtc)
{
	int ret = 0;

	mtx_lock(&dev->vbl_lock);
	/* Going from 0->1 means we have to enable interrupts again */
	if (atomic_add_return(1, &dev->vblank_refcount[crtc]) == 1) {
		mtx_lock(&dev->vblank_time_lock);
		if (!dev->vblank_enabled[crtc]) {
			/* Enable vblank irqs under vblank_time_lock protection.
			 * All vblank count & timestamp updates are held off
			 * until we are done reinitializing master counter and
			 * timestamps. Filtercode in drm_handle_vblank() will
			 * prevent double-accounting of same vblank interval.
			 */
			ret = -dev->driver->enable_vblank(dev, crtc);
			DRM_DEBUG("enabling vblank on crtc %d, ret: %d\n",
				  crtc, ret);
			if (ret)
				atomic_dec(&dev->vblank_refcount[crtc]);
			else {
				dev->vblank_enabled[crtc] = 1;
				drm_update_vblank_count(dev, crtc);
			}
		}
		mtx_unlock(&dev->vblank_time_lock);
	} else {
		if (!dev->vblank_enabled[crtc]) {
			atomic_dec(&dev->vblank_refcount[crtc]);
			ret = EINVAL;
		}
	}
	mtx_unlock(&dev->vbl_lock);

	return ret;
}

/**
 * drm_vblank_put - give up ownership of vblank events
 * @dev: DRM device
 * @crtc: which counter to give up
 *
 * Release ownership of a given vblank counter, turning off interrupts
 * if possible. Disable interrupts after drm_vblank_offdelay milliseconds.
 */
void drm_vblank_put(struct drm_device *dev, int crtc)
{
	KASSERT(atomic_read(&dev->vblank_refcount[crtc]) != 0,
	    ("Too many drm_vblank_put for crtc %d", crtc));

	/* Last user schedules interrupt disable */
	if (atomic_dec_and_test(&dev->vblank_refcount[crtc]) &&
	    (drm_vblank_offdelay > 0))
		callout_reset(&dev->vblank_disable_callout,
		    (drm_vblank_offdelay * DRM_HZ) / 1000,
		    vblank_disable_fn, dev);
}

void drm_vblank_off(struct drm_device *dev, int crtc)
{
	struct drm_pending_vblank_event *e, *t;
	struct timeval now;
	unsigned int seq;

	mtx_lock(&dev->vbl_lock);
	vblank_disable_and_save(dev, crtc);
	mtx_lock(&dev->event_lock);
	wakeup(&dev->_vblank_count[crtc]);

	/* Send any queued vblank events, lest the natives grow disquiet */
	seq = drm_vblank_count_and_time(dev, crtc, &now);
	list_for_each_entry_safe(e, t, &dev->vblank_event_list, base.link) {
		if (e->pipe != crtc)
			continue;
		DRM_DEBUG("Sending premature vblank event on disable: \
			  wanted %d, current %d\n",
			  e->event.sequence, seq);

		e->event.sequence = seq;
		e->event.tv_sec = now.tv_sec;
		e->event.tv_usec = now.tv_usec;
		drm_vblank_put(dev, e->pipe);
		list_move_tail(&e->base.link, &e->base.file_priv->event_list);
		drm_event_wakeup(&e->base);
		CTR3(KTR_DRM, "vblank_event_delivered %d %d %d",
		    e->base.pid, e->pipe, e->event.sequence);
	}

	mtx_unlock(&dev->event_lock);
	mtx_unlock(&dev->vbl_lock);
}

/**
 * drm_vblank_pre_modeset - account for vblanks across mode sets
 * @dev: DRM device
 * @crtc: CRTC in question
 * @post: post or pre mode set?
 *
 * Account for vblank events across mode setting events, which will likely
 * reset the hardware frame counter.
 */
void drm_vblank_pre_modeset(struct drm_device *dev, int crtc)
{
	/* vblank is not initialized (IRQ not installed ?) */
	if (!dev->num_crtcs)
		return;
	/*
	 * To avoid all the problems that might happen if interrupts
	 * were enabled/disabled around or between these calls, we just
	 * have the kernel take a reference on the CRTC (just once though
	 * to avoid corrupting the count if multiple, mismatch calls occur),
	 * so that interrupts remain enabled in the interim.
	 */
	if (!dev->vblank_inmodeset[crtc]) {
		dev->vblank_inmodeset[crtc] = 0x1;
		if (drm_vblank_get(dev, crtc) == 0)
			dev->vblank_inmodeset[crtc] |= 0x2;
	}
}

void drm_vblank_post_modeset(struct drm_device *dev, int crtc)
{

	if (dev->vblank_inmodeset[crtc]) {
		mtx_lock(&dev->vbl_lock);
		dev->vblank_disable_allowed = 1;
		mtx_unlock(&dev->vbl_lock);

		if (dev->vblank_inmodeset[crtc] & 0x2)
			drm_vblank_put(dev, crtc);

		dev->vblank_inmodeset[crtc] = 0;
	}
}

/**
 * drm_modeset_ctl - handle vblank event counter changes across mode switch
 * @DRM_IOCTL_ARGS: standard ioctl arguments
 *
 * Applications should call the %_DRM_PRE_MODESET and %_DRM_POST_MODESET
 * ioctls around modesetting so that any lost vblank events are accounted for.
 *
 * Generally the counter will reset across mode sets.  If interrupts are
 * enabled around this call, we don't have to do anything since the counter
 * will have already been incremented.
 */
int drm_modeset_ctl(struct drm_device *dev, void *data,
		    struct drm_file *file_priv)
{
	struct drm_modeset_ctl *modeset = data;
	unsigned int crtc;

	/* If drm_vblank_init() hasn't been called yet, just no-op */
	if (!dev->num_crtcs)
		return 0;

	crtc = modeset->crtc;
	if (crtc >= dev->num_crtcs)
		return -EINVAL;

	switch (modeset->cmd) {
	case _DRM_PRE_MODESET:
		drm_vblank_pre_modeset(dev, crtc);
		break;
	case _DRM_POST_MODESET:
		drm_vblank_post_modeset(dev, crtc);
		break;
	default:
		return -EINVAL;
		break;
	}

	return 0;
}

static void
drm_vblank_event_destroy(struct drm_pending_event *e)
{

	free(e, DRM_MEM_VBLANK);
}

static int drm_queue_vblank_event(struct drm_device *dev, int pipe,
				  union drm_wait_vblank *vblwait,
				  struct drm_file *file_priv)
{
	struct drm_pending_vblank_event *e;
	struct timeval now;
	unsigned int seq;
	int ret;

	e = malloc(sizeof *e, DRM_MEM_VBLANK, M_WAITOK | M_ZERO);

	e->pipe = pipe;
	e->base.pid = curproc->p_pid;
	e->event.base.type = DRM_EVENT_VBLANK;
	e->event.base.length = sizeof e->event;
	e->event.user_data = vblwait->request.signal;
	e->base.event = &e->event.base;
	e->base.file_priv = file_priv;
	e->base.destroy = drm_vblank_event_destroy;

	mtx_lock(&dev->event_lock);

	if (file_priv->event_space < sizeof e->event) {
		ret = EBUSY;
		goto err_unlock;
	}

	file_priv->event_space -= sizeof e->event;
	seq = drm_vblank_count_and_time(dev, pipe, &now);

	if ((vblwait->request.type & _DRM_VBLANK_NEXTONMISS) &&
	    (seq - vblwait->request.sequence) <= (1 << 23)) {
		vblwait->request.sequence = seq + 1;
		vblwait->reply.sequence = vblwait->request.sequence;
	}

	DRM_DEBUG("event on vblank count %d, current %d, crtc %d\n",
		  vblwait->request.sequence, seq, pipe);

	CTR4(KTR_DRM, "vblank_event_queued %d %d rt %x %d", curproc->p_pid, pipe,
	    vblwait->request.type, vblwait->request.sequence);

	e->event.sequence = vblwait->request.sequence;
	if ((seq - vblwait->request.sequence) <= (1 << 23)) {
		e->event.sequence = seq;
		e->event.tv_sec = now.tv_sec;
		e->event.tv_usec = now.tv_usec;
		drm_vblank_put(dev, pipe);
		list_add_tail(&e->base.link, &e->base.file_priv->event_list);
		drm_event_wakeup(&e->base);
		vblwait->reply.sequence = seq;
		CTR3(KTR_DRM, "vblank_event_wakeup p1 %d %d %d", curproc->p_pid,
		    pipe, vblwait->request.sequence);
	} else {
		/* drm_handle_vblank_events will call drm_vblank_put */
		list_add_tail(&e->base.link, &dev->vblank_event_list);
		vblwait->reply.sequence = vblwait->request.sequence;
	}

	mtx_unlock(&dev->event_lock);

	return 0;

err_unlock:
	mtx_unlock(&dev->event_lock);
	free(e, DRM_MEM_VBLANK);
	drm_vblank_put(dev, pipe);
	return ret;
}

/**
 * Wait for VBLANK.
 *
 * \param inode device inode.
 * \param file_priv DRM file private.
 * \param cmd command.
 * \param data user argument, pointing to a drm_wait_vblank structure.
 * \return zero on success or a negative number on failure.
 *
 * This function enables the vblank interrupt on the pipe requested, then
 * sleeps waiting for the requested sequence number to occur, and drops
 * the vblank interrupt refcount afterwards. (vblank irq disable follows that
 * after a timeout with no further vblank waits scheduled).
 */
int drm_wait_vblank(struct drm_device *dev, void *data,
		    struct drm_file *file_priv)
{
	union drm_wait_vblank *vblwait = data;
	int ret;
	unsigned int flags, seq, crtc, high_crtc;

	if (/*(!drm_dev_to_irq(dev)) || */(!dev->irq_enabled))
		return (EINVAL);

	if (vblwait->request.type & _DRM_VBLANK_SIGNAL)
		return (EINVAL);

	if (vblwait->request.type &
	    ~(_DRM_VBLANK_TYPES_MASK | _DRM_VBLANK_FLAGS_MASK |
	      _DRM_VBLANK_HIGH_CRTC_MASK)) {
		DRM_ERROR("Unsupported type value 0x%x, supported mask 0x%x\n",
			  vblwait->request.type,
			  (_DRM_VBLANK_TYPES_MASK | _DRM_VBLANK_FLAGS_MASK |
			   _DRM_VBLANK_HIGH_CRTC_MASK));
		return (EINVAL);
	}

	flags = vblwait->request.type & _DRM_VBLANK_FLAGS_MASK;
	high_crtc = (vblwait->request.type & _DRM_VBLANK_HIGH_CRTC_MASK);
	if (high_crtc)
		crtc = high_crtc >> _DRM_VBLANK_HIGH_CRTC_SHIFT;
	else
		crtc = flags & _DRM_VBLANK_SECONDARY ? 1 : 0;
	if (crtc >= dev->num_crtcs)
		return (EINVAL);

	ret = drm_vblank_get(dev, crtc);
	if (ret) {
		DRM_DEBUG("failed to acquire vblank counter, %d\n", ret);
		return (ret);
	}
	seq = drm_vblank_count(dev, crtc);

	switch (vblwait->request.type & _DRM_VBLANK_TYPES_MASK) {
	case _DRM_VBLANK_RELATIVE:
		vblwait->request.sequence += seq;
		vblwait->request.type &= ~_DRM_VBLANK_RELATIVE;
	case _DRM_VBLANK_ABSOLUTE:
		break;
	default:
		ret = (EINVAL);
		goto done;
	}

	if (flags & _DRM_VBLANK_EVENT) {
		/* must hold on to the vblank ref until the event fires
		 * drm_vblank_put will be called asynchronously
		 */
		return drm_queue_vblank_event(dev, crtc, vblwait, file_priv);
	}

	if ((flags & _DRM_VBLANK_NEXTONMISS) &&
	    (seq - vblwait->request.sequence) <= (1<<23)) {
		vblwait->request.sequence = seq + 1;
	}

	dev->last_vblank_wait[crtc] = vblwait->request.sequence;
	mtx_lock(&dev->vblank_time_lock);
	while (((drm_vblank_count(dev, crtc) - vblwait->request.sequence) >
	    (1 << 23)) && dev->irq_enabled) {
		/*
		 * The wakeups from the drm_irq_uninstall() and
		 * drm_vblank_off() may be lost there since vbl_lock
		 * is not held.  Then, the timeout will wake us; the 3
		 * seconds delay should not be a problem for
		 * application when crtc is disabled or irq
		 * uninstalled anyway.
		 */
		ret = msleep(&dev->_vblank_count[crtc], &dev->vblank_time_lock,
		    PCATCH, "drmvbl", 3 * hz);
		if (ret != 0)
			break;
	}
	mtx_unlock(&dev->vblank_time_lock);
	if (ret != EINTR) {
		struct timeval now;
		long reply_seq;

		reply_seq = drm_vblank_count_and_time(dev, crtc, &now);
		CTR5(KTR_DRM, "wait_vblank %d %d rt %x success %d %d",
		    curproc->p_pid, crtc, vblwait->request.type,
		    vblwait->request.sequence, reply_seq);
		vblwait->reply.sequence = reply_seq;
		vblwait->reply.tval_sec = now.tv_sec;
		vblwait->reply.tval_usec = now.tv_usec;
	} else {
		CTR5(KTR_DRM, "wait_vblank %d %d rt %x error %d %d",
		    curproc->p_pid, crtc, vblwait->request.type, ret,
		    vblwait->request.sequence);
	}

done:
	drm_vblank_put(dev, crtc);
	return ret;
}

void drm_handle_vblank_events(struct drm_device *dev, int crtc)
{
	struct drm_pending_vblank_event *e, *t;
	struct timeval now;
	unsigned int seq;

	seq = drm_vblank_count_and_time(dev, crtc, &now);
	CTR2(KTR_DRM, "drm_handle_vblank_events %d %d", seq, crtc);

	mtx_lock(&dev->event_lock);

	list_for_each_entry_safe(e, t, &dev->vblank_event_list, base.link) {
		if (e->pipe != crtc)
			continue;
		if ((seq - e->event.sequence) > (1<<23))
			continue;

		e->event.sequence = seq;
		e->event.tv_sec = now.tv_sec;
		e->event.tv_usec = now.tv_usec;
		drm_vblank_put(dev, e->pipe);
		list_move_tail(&e->base.link, &e->base.file_priv->event_list);
		drm_event_wakeup(&e->base);
		CTR3(KTR_DRM, "vblank_event_wakeup p2 %d %d %d", e->base.pid,
		    e->pipe, e->event.sequence);
	}

	mtx_unlock(&dev->event_lock);
}

/**
 * drm_handle_vblank - handle a vblank event
 * @dev: DRM device
 * @crtc: where this event occurred
 *
 * Drivers should call this routine in their vblank interrupt handlers to
 * update the vblank counter and send any signals that may be pending.
 */
bool drm_handle_vblank(struct drm_device *dev, int crtc)
{
	u32 vblcount;
	int64_t diff_ns;
	struct timeval tvblank;

	if (!dev->num_crtcs)
		return false;

	/* Need timestamp lock to prevent concurrent execution with
	 * vblank enable/disable, as this would cause inconsistent
	 * or corrupted timestamps and vblank counts.
	 */
	mtx_lock(&dev->vblank_time_lock);

	/* Vblank irq handling disabled. Nothing to do. */
	if (!dev->vblank_enabled[crtc]) {
		mtx_unlock(&dev->vblank_time_lock);
		return false;
	}

	/* Fetch corresponding timestamp for this vblank interval from
	 * driver and store it in proper slot of timestamp ringbuffer.
	 */

	/* Get current timestamp and count. */
	vblcount = atomic_read(&dev->_vblank_count[crtc]);
	drm_get_last_vbltimestamp(dev, crtc, &tvblank, DRM_CALLED_FROM_VBLIRQ);

	/* Compute time difference to timestamp of last vblank */
	diff_ns = timeval_to_ns(&tvblank) -
		  timeval_to_ns(&vblanktimestamp(dev, crtc, vblcount));

	/* Update vblank timestamp and count if at least
	 * DRM_REDUNDANT_VBLIRQ_THRESH_NS nanoseconds
	 * difference between last stored timestamp and current
	 * timestamp. A smaller difference means basically
	 * identical timestamps. Happens if this vblank has
	 * been already processed and this is a redundant call,
	 * e.g., due to spurious vblank interrupts. We need to
	 * ignore those for accounting.
	 */
	if (abs64(diff_ns) > DRM_REDUNDANT_VBLIRQ_THRESH_NS) {
		/* Store new timestamp in ringbuffer. */
		vblanktimestamp(dev, crtc, vblcount + 1) = tvblank;

		/* Increment cooked vblank count. This also atomically commits
		 * the timestamp computed above.
		 */
		atomic_inc(&dev->_vblank_count[crtc]);
	} else {
		DRM_DEBUG("crtc %d: Redundant vblirq ignored. diff_ns = %d\n",
			  crtc, (int) diff_ns);
	}

	wakeup(&dev->_vblank_count[crtc]);
	drm_handle_vblank_events(dev, crtc);

	mtx_unlock(&dev->vblank_time_lock);
	return true;
}
