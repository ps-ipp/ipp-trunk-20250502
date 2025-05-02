camRun METADATA
    cam_id      S64         0       # Primary Key AUTO_INCREMENT
    chip_id     S64         0       # Key INDEX(cam_id, chip_id) fkey(chip_id) ref chipRun(chip_id)
    state       STR         64      # key
    workdir     STR         255 
    workdir_state STR       64      # key
    label       STR         64      # key
    data_group  STR         64      # key
    dist_group  STR         64      # key
    reduction   STR         64
    expgroup    STR         64      # key
    dvodb       STR         255
    tess_id     STR         64
    end_stage   STR         64      # Key
    magicked    S64         0
    software_ver    STR         16
    maskfrac_ref_npix S32       0
    maskfrac_ref_static F32	    0.0
    maskfrac_ref_dynamic F32    0.0
    maskfrac_ref_magic  F32     0.0
    maskfrac_ref_advisory F32   0.0
    maskfrac_max_npix S32       0
    maskfrac_max_static F32	    0.0
    maskfrac_max_dynamic F32    0.0
    maskfrac_max_magic  F32     0.0
    maskfrac_max_advisory F32   0.0
    note        STR         255
END

camProcessedExp METADATA
# the camPendingExp row gets deleted so we can not put a fkey on cam_id
    cam_id         S64      0       # Primary Key
    uri            STR      255     # fkey(cam_id) ref camRun(cam_id)

    bg             F32      0.0
    bg_stdev       F32      0.0
    bg_mean_stdev  F32      0.0
    bias	   F32	    0.0
    bias_stdev     F32	    0.0
    fringe_0       F32      0.0
    fringe_1       F32      0.0
    fringe_2       F32      0.0
    sigma_ra       F32      0.0
    sigma_dec      F32      0.0
    ap_resid       F32	    0.0
    ap_resid_stdev F32	    0.0

    zpt_obs        F32      0.0
    zpt_stdev      F32      0.0
    zpt_lq         F32      0.0
    zpt_uq         F32      0.0

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

    dtime_script   F32      0.0
    dtime_astrom   F32      0.0
    dtime_addstar  F32      0.0

    hostname       STR      64
    n_stars        S32      0
    n_psfstars     S32	    0
    n_iqstars      S32	    0
    n_extended     S32      0
    n_cr           S32      0
    n_astrom       S32      0
    path_base      STR      255
    fault          S16      0       # Key NOT NULL
    software_ver    STR         16
    maskfrac_ref_npix S32       0
    maskfrac_ref_static F32	    0.0
    maskfrac_ref_dynamic F32    0.0
    maskfrac_ref_magic  F32     0.0
    maskfrac_ref_advisory F32   0.0
    maskfrac_max_npix S32       0
    maskfrac_max_static F32	    0.0
    maskfrac_max_dynamic F32    0.0
    maskfrac_max_magic  F32     0.0
    maskfrac_max_advisory F32   0.0
    deteff         F32      0
    deteff_err     F32      0
    deteff_lq      F32      0
    deteff_uq      F32      0
    quality        S16      0
    background_model  S16   0
    astrom_chips   S64      0
    ast_r0         F32      0.0 
    ast_d0         F32      0.0 
    ast_t0         F32      0.0 
    ast_s0         F32      0.0 
    ast_rs         F32      0.0 
    ast_ds         F32      0.0 
END

camMask METADATA
    label       STR         64      # Primary Key
END
