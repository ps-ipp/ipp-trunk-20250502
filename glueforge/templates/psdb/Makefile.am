template_files = \
	COPYING \
	metadata \
	metadata_from_params \
	metadata_from_struct \
	lookup_from_metadata \
    alloc.tt \
    alloc_at.tt \
    alloc_c.tt \
    alloc_h.tt \
    autogen_sh.tt \
    bootstrap_sh.tt \
    cleanup.tt \
    cleanup_at.tt \
    cleanup_c.tt \
    cleanup_h.tt \
    code.tt \
    config.pl \
    configure_ac.tt \
    createtable.tt \
    createtable_at.tt \
    createtable_c.tt \
    createtable_h.tt \
    dbcleanup_c.tt \
    dbsetup_c.tt \
    delete.tt \
    delete_h.tt \
    deleteobject.tt \
    deleteobject_h.tt \
    deleterowobjects.tt \
    deleterowobjects_h.tt \
    doxyfile_in.tt \
    droptable.tt \
    droptable_at.tt \
    droptable_c.tt \
    droptable_h.tt \
    header.tt \
    init.tt \
    init_at.tt \
    init_c.tt \
    init_h.tt \
    insert.tt \
    insert_at.tt \
    insert_c.tt \
    insert_h.tt \
    insertfits.tt \
    insertfits_at.tt \
    insertfits_c.tt \
    insertfits_h.tt \
    insertobject.tt \
    insertobject_at.tt \
    insertobject_c.tt \
    insertobject_h.tt \
    insertobjects.tt \
    insertobjects_h.tt \
    makefile.tt \
    metadatafromobject.tt \
    metadatafromobject_at.tt \
    metadatafromobject_c.tt \
    metadatafromobject_h.tt \
    object_h.tt \
    objectfrommetadata.tt \
    objectfrommetadata_at.tt \
    objectfrommetadata_c.tt \
    objectfrommetadata_h.tt \
    pkgconfig_pc_in.tt \
    printmetadata.tt \
    printmetadata_h.tt \
    printmetadataraw.tt \
    printmetadataraw_h.tt \
    printmetadatas.tt \
    printmetadatas_h.tt \
    printmetadatasraw.tt \
    printmetadatasraw_h.tt \
    printobject.tt \
    printobject_h.tt \
    printobjects.tt \
    printobjects_h.tt \
    selectrowobjects.tt \
    selectrowobjects_h.tt \
    selectrowsfits.tt \
    selectrowsfits_at.tt \
    selectrowsfits_c.tt \
    selectrowsfits_h.tt \
    src_makefile_am.tt \
    tests_makefile_am.tt \
    testsuite_at.tt \
    top_makefile_am.tt

templatedir = $(datadir)/@PACKAGE_NAME@/templates
psdbtemplatedir = $(templatedir)/psdb
psdbtemplate_DATA = $(template_files)

install-data-hook:
	chmod 0755 $(templatedir) $(psdbtemplatedir)

EXTRA_DIST = $(template_files)
