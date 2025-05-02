chipRun METADATA
    chip_id     S64         0       # Primary Key AUTO_INCREMENT
    exp_id      S64         64      # Key INDEX(chip_id, exp_id) fkey (exp_id) ref rawExp(exp_id)
    state       STR         64      # Key
    workdir     STR         255 
    workdir_state STR       64      # Key
    label       STR         64      # Key
    data_group  STR         64      # Key
    dist_group  STR         64      # Key
    reduction   STR         64      # Reduction class
    expgroup    STR         64      # Key
    dvodb       STR         255
    tess_id     STR         64
    end_stage   STR         64      # Key
    magicked    S64         0
    software_ver    STR         16
    maskfrac_npix F32       0.0
    maskfrac_static F32	    0.0
    maskfrac_dynamic F32    0.0
    maskfrac_magic  F32     0.0
    maskfrac_advisory F32   0.0
    update_mode S16         0  
    note        STR         255
END

chipImfile METADATA
    chip_id     S64         0       # Primary Key fkey (chip_id) ref chipRun (chip_id)
    class_id    STR         64      # Primary Key
    chip_imfile_id   S64     0       # Key AUTO_INCREMENT
END

chipProcessedImfile METADATA
    chip_id         S64     0       # Primary Key fkey (chip_id, exp_id) ref chipRun(chip_id, exp_id)
    exp_id          S64     64      # Primary Key fkey (exp_id, class_id) ref rawImfile(exp_id, class_id)
    class_id        STR     64      # Primary Key
    data_state      STR     64      # Key
    uri             STR     255
    bg              F32     0.0
    bg_stdev        F32     0.0
    bg_mean_stdev   F32     0.0
    bias	    F32	    0.0
    bias_stdev      F32	    0.0
    fringe_0        F32     0.0
    fringe_1        F32     0.0
    fringe_2        F32     0.0
    ap_resid        F32	    0.0
    ap_resid_stdev  F32	    0.0

    fwhm_major      F32     0.0
    fwhm_major_lq   F32     0.0
    fwhm_major_uq   F32     0.0

    fwhm_minor      F32     0.0
    fwhm_minor_lq   F32     0.0
    fwhm_minor_uq   F32     0.0

    iq_fwhm_major     F32     0.0        
    iq_fwhm_major_err F32     0.0        
    iq_fwhm_minor     F32     0.0        
    iq_fwhm_minor_err F32     0.0        

    iq_m2           F32     0.0        
    iq_m2_err       F32     0.0        
    iq_m2_lq        F32     0.0        
    iq_m2_uq        F32     0.0        

    iq_m2c          F32     0.0        
    iq_m2c_err      F32     0.0        
    iq_m2c_lq       F32     0.0        
    iq_m2c_uq       F32     0.0        

    iq_m2s          F32     0.0        
    iq_m2s_err      F32     0.0        
    iq_m2s_lq       F32     0.0        
    iq_m2s_uq       F32     0.0        

    iq_m3           F32     0.0        
    iq_m3_err       F32     0.0        
    iq_m3_lq        F32     0.0        
    iq_m3_uq        F32     0.0        

    iq_m4           F32     0.0        
    iq_m4_err       F32     0.0        
    iq_m4_lq        F32     0.0        
    iq_m4_uq        F32     0.0        

    dtime_detrend   F32     0.0
    dtime_photom    F32     0.0
    dtime_total     F32     0.0
    dtime_script    F32     0.0

    hostname        STR     64
    n_stars         S32	    0
    n_psfstars      S32	    0
    n_iqstars       S32	    0
    n_extended      S32	    0
    n_cr            S32	    0
    path_base       STR     255
    fault           S16     0       # Key NOT NULL
    quality         S16     0
    magicked        S64     0
    software_ver    STR         16
    maskfrac_npix S32       0
    maskfrac_static F32	    0.0
    maskfrac_dynamic F32    0.0
    maskfrac_magic  F32     0.0
    maskfrac_advisory F32   0.0
    deteff_magref   F32     0.0
END

chipMask METADATA
    label       STR         64      # Primary Key
END
