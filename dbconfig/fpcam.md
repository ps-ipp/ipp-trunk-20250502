# Forced Camera Photometry (fpcam)

# For this stage, we are processing an exposure by applying the camera
# stage astrometric calibration to force the photometry of sources
# defined in the dvo database (dvodb) on the processed chip images.
# For a given exposure, it should not be necessary to select the chip
# analysis which is connected to the desired camera analysis

fpcamRun METADATA
    fpcam_id      S64       0       # Primary Key AUTO_INCREMENT
    chip_id       S64       0       # Key INDEX(cam_id, chip_id) fkey(chip_id) ref chipRun(chip_id)
    cam_id        S64       0       # Key INDEX(cam_id, chip_id) fkey(cam_id)  ref camRun(cam_id)
    state         STR       64      # key
    workdir       STR       255 
    workdir_state STR       64
    label         STR       64      # key
    data_group    STR       64      # key
    dist_group    STR       64
    reduction     STR       64
    dvodb         STR       255
    note          STR       255
END

fpcamProcessedExp METADATA
    fpcam_id       S64      0       # Primary Key fkey(fpcam_id) ref fpcamRun(fpcam_id)
    path_base      STR      255

    zpt_obs        F32      0.0
    zpt_stdev      F32      0.0
    zpt_lq         F32      0.0
    zpt_uq         F32      0.0

    dtime_script   F32      0.0

    hostname       STR      64
    n_stars        S32      0
    fault          S16      0       # Key NOT NULL
    epoch          UTC      0001-01-01T00:00:00Z

    software_ver   STR      16
    deteff_obs     F32      0
    deteff_err     F32      0
    deteff_lq      F32      0
    deteff_uq      F32      0

    quality        S16      0
END
