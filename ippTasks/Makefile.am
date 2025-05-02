task_files = \
	detrend.norm.pro \
	detrend.process.pro \
	detrend.reject.pro \
	detrend.resid.pro \
	detrend.stack.pro \
	detrend.correct.pro \
	detrend.cleanup.pro \
	flatcorr.pro \
	automate.pro \
	pantasks.pro \
	register.pro \
	chip.pro \
	camera.pro \
	addstar.pro \
	fake.pro \
	warp.pro \
	magic.pro \
	magic.cleanup.pro \
	destreak.pro \
	destreak.cleanup.pro \
	diff.pro \
	skycal.pro \
	stack.pro \
	staticsky.pro \
	summit.copy.pro \
	survey.pro \
	replicate.pro \
	rawcheck.pro \
	diskbalance.pro \
	dist.pro \
	dist.cleanup.pro \
	rcserver.pro \
	pstamp.pro \
	receive.pro \
	publish.pro \
	dqstats.pro \
	science.cleanup.pro \
	minidvodb.pro \
	minidvodbcopy.pro \
	mergedvodb.pro \
	nightly_science.pro \
	burntool.pro \
	lossy_compress.pro \
	background.pro \
	diffphot.pro \
	lap.pro \
	lapgroup.pro \
	vp.pro \
	bg.regeneration.pro \
	remote.pro \
	fpcamera.pro \
	fullforce.pro

other_files = \
	ipphosts.manoa.config \
	ipphosts.mhpcc.config \
	site.manoa.pro \
	site.mhpcc.pro \
	surveys.mhpcc.config \
	simtest.pro \
	simtest.basic.config \
	simtest.basic.auto \
	simtest.nebulous.basic.auto \
	simtest.detverify.config \
	simtest.detverify.auto \
	simtest.flatcorr.config \
	simtest.flatcorr.auto \
	simtest.ctemask.config \
	simtest.ctemask.auto \
	simtest.stack.config \
	simtest.stack.auto

pantasksdir = $(datadir)/pantasks
ipptasksdir = $(pantasksdir)/modules
ipptasks_DATA = $(task_files) $(other_files)

install-data-hook:
	chmod 0755 $(panstasksdir) $(ipptasksdir)

EXTRA_DIST = $(task_files) $(other_files)

ACLOCAL_AMFLAGS = -I m4
