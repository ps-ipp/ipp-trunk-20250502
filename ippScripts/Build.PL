use Module::Build;
# See perldoc Module::Build for details of how this works

Module::Build->new(
    module_name         => 'ippscripts',
    dist_version        => '0.01',
    author              => 'Joshua Hoblitt <jhoblitt@cpan.org>',
    license             => 'gpl',
    create_makefile_pl  => 'passthrough',
    requires            => {
        'Astro::FITS::CFITSIO'      => 0,
        'Cache::File'               => 0,
        'Carp'                      => 0,
        'Data::Dumper'              => 0,
        'File::Basename'            => 0,
        'File::Spec'                => 0,
        'File::Temp'                => 0,
        'Getopt::Long'              => 0,
        'IPC::Cmd'                  => 0,
        'IPC::Run'                  => 0,
        'Math::Trig'                => 0,
        'Pod::Usage'                => 0,
        'PS::IPP::Config'           => '1.01',
        'PS::IPP::Metadata::Config' => 0,
        'Statistics::Descriptive'   => 0,
        'Storable'                  => 0,
        'Sys::Hostname'             => 0,
    },
    recommends          => {
        'Test::Distribution'    => '1.22',
    },
    script_files        => [qw(
        scripts/ipp_datapath.pl
        scripts/ipp_filename.pl
        scripts/mdc2list.pl
        scripts/register_imfile.pl
        scripts/register_exp.pl
        scripts/detrend_correct_imfile.pl
        scripts/detrend_process_imfile.pl
        scripts/detrend_process_exp.pl
        scripts/detrend_stack.pl
        scripts/detrend_norm_calc.pl
        scripts/detrend_norm_apply.pl
        scripts/detrend_norm_exp.pl
        scripts/detrend_resid_imfile.pl
        scripts/detrend_resid_exp.pl
        scripts/detrend_reject_exp.pl
        scripts/flatcorr_proc.pl
        scripts/chip_imfile.pl
        scripts/camera_exp.pl
        scripts/addstar_multi_run.pl
        scripts/addstar_run.pl
        scripts/fake_imfile.pl
        scripts/warp_overlap.pl
        scripts/warp_skycell.pl
        scripts/diff_skycell.pl
        scripts/stack_skycell.pl
        scripts/staticsky.pl
        scripts/destreak_restore_camera.pl
        scripts/magic_tree.pl
        scripts/magic_process.pl
        scripts/magic_cleanup.pl
        scripts/magic_destreak.pl
        scripts/magic_destreak_revert.pl
        scripts/magic_destreak_cleanup.pl
        scripts/magic_destreak_defineruns.pl
        scripts/minidvodb_createdb.pl
        scripts/minidvodb_merge.pl
        scripts/minidvodb_premerge.pl
        scripts/minidvodb_copy.pl
        scripts/mergedvodb_copy.pl
        scripts/mergedvodb_merge.pl
        scripts/mergedvodb_queue.pl
        scripts/ippdb.pl
        scripts/ipp_cleanup.pl
        scripts/ipp_inject_fileset.pl
        scripts/ipp_inject_expname.pl
        scripts/ipp_serial_inject.pl
        scripts/ipp_serial_inject_into_nebulous.pl
        scripts/ipp_serial_inject_split.pl
        scripts/ipp_serial_inject_mosaic.pl
        scripts/ipp_serial_register.pl
        scripts/ipp_serial_detrend.pl
        scripts/ipp_serial_chip.pl
        scripts/ipp_serial_camera.pl
        scripts/ipp_serial_warp.pl
        scripts/ipp_serial_diff.pl
        scripts/ipp_serial_stack.pl
        scripts/ipp_serial_mops.pl
        scripts/ipp_serial_mops_warps.pl
        scripts/ipp_serial_magic.pl
        scripts/ipp_mops_translate.pl
        scripts/ipp_simulation_data.pl
        scripts/isp_trans.pl
        scripts/ds9_cmf_regions.pl
        scripts/ipp_detrend_combine.pl
        scripts/summit_copy.pl
        scripts/ipp_image_path.pl
        scripts/dist_cleanup.pl
        scripts/dist_component.pl
        scripts/dist_bundle.pl
        scripts/dist_advancerun.pl
        scripts/dist_fix_file.pl
        scripts/dist_make_fileset.pl
        scripts/dist_defineruns.pl
        scripts/receive_source.pl
        scripts/receive_fileset.pl
        scripts/receive_file.pl
        scripts/receive_advance.pl
        scripts/receive_setstatus.pl
        scripts/rcserver_checkstatus.pl
        scripts/publish_file.pl
        scripts/dqstats_bundle.pl
        scripts/whichimage
        scripts/automate_stacks.pl
        scripts/nightly_science.pl
        scripts/lossy_compress_imfile.pl
        scripts/rawcheck.pl
        scripts/permcheck.pl
        scripts/ipp_apply_burntool.pl
        scripts/ipp_apply_burntool_single.pl
        scripts/ipp_apply_burntool_fix.pl
        scripts/make_burntool_pcontrol.pl
        scripts/bundle_detrends.pl
        scripts/ipp_cluster_load_monitor.pl
        scripts/background_chip.pl
        scripts/background_warp.pl
        scripts/skycell_jpeg.pl
        scripts/diffphot.pl
        scripts/lap_science.pl
        scripts/lap_queue_diff.pl
        scripts/videophot_process.pl
        scripts/listvideocells.pl
        scripts/skycalibration.pl
        scripts/regenerate_background.pl
        scripts/relgroup_exp_list.pl
        scripts/queuestaticsky.pl
        scripts/psphot_fullforce_warp.pl
        scripts/psphot_fullforce_summary.pl
        scripts/stack_bkg_mk_mdc.pl
        scripts/sc_mk_stack_mdc.pl
        scripts/sc_mk_staticsky_mdc.pl
        scripts/sc_check_diff.pl
        scripts/sc_prepare_camera.pl
        scripts/sc_prepare_chip.pl
        scripts/sc_prepare_stack.pl
        scripts/sc_prepare_staticsky.pl
        scripts/sc_prepare_diff.pl
        scripts/sc_prepare_ff.pl
        scripts/sc_prepare_warp.pl
        scripts/sc_prepare_run.pl
        scripts/sc_remote_exec.pl
        scripts/sc_remote_poll.pl
        scripts/sc_retry_return.pl
        scripts/sc_transfer_tool.pl
        scripts/remote_md5sum.pl
        scripts/fpcamera_exp.pl
    )],
    dist_abstract => 'Scripts for running the Pan-STARRS IPP',
    dist_author => 'Paul Price <price@ifa.hawaii.edu>',
)->create_build_script;
