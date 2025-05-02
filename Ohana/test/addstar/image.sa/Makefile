
DVOCHECK = ./dvocheck

all: megacam asca

asca: asca.raw asca.mef asca.split

asca.raw: asca.loneos.raw asca.elixir.raw asca.pmtest.raw asca.panstarrs.raw
asca.mef: asca.loneos.mef asca.elixir.mef asca.pmtest.mef asca.panstarrs.mef
asca.split: asca.loneos.split asca.elixir.split asca.pmtest.split asca.panstarrs.split

######
asca.L1:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	$(DVOCHECK) catdir.asca.L1.raw asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	$(DVOCHECK) catdir.asca.L1.mef asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.L1.split asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L1.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar -resort -D CATDIR catdir.asca.L1.update
	$(DVOCHECK) catdir.asca.L1.update asca

######
asca.L2:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	$(DVOCHECK) catdir.asca.L2.raw asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	$(DVOCHECK) catdir.asca.L2.mef asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.L2.split asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 2 -D CATDIR catdir.asca.L2.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar -resort -D CATDIR catdir.asca.L2.update
	$(DVOCHECK) catdir.asca.L2.update asca

######
asca.L3:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	$(DVOCHECK) catdir.asca.L3.raw asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	$(DVOCHECK) catdir.asca.L3.mef asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.L3.split asca
#
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D SKY_DEPTH 3 -D CATDIR catdir.asca.L3.update -D CATFORMAT ELIXIR -D CATMODE SPLIT -update -missed -closest
	addstar -resort -D CATDIR catdir.asca.L3.update
	$(DVOCHECK) catdir.asca.L3.update asca

######
asca.loneos.raw:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.raw -D CATFORMAT LONEOS -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.raw -D CATFORMAT LONEOS -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.raw -D CATFORMAT LONEOS -D CATMODE RAW
	$(DVOCHECK) catdir.asca.loneos.raw asca

asca.elixir.raw:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	$(DVOCHECK) catdir.asca.elixir.raw asca

asca.pmtest.raw:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.raw -D CATFORMAT PMTEST -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.raw -D CATFORMAT PMTEST -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.raw -D CATFORMAT PMTEST -D CATMODE RAW
	$(DVOCHECK) catdir.asca.pmtest.raw asca

asca.panstarrs.raw:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.raw -D CATFORMAT PANSTARRS -D CATMODE RAW
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.raw -D CATFORMAT PANSTARRS -D CATMODE RAW
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.raw -D CATFORMAT PANSTARRS -D CATMODE RAW
	$(DVOCHECK) catdir.asca.panstarrs.raw asca

#######
asca.loneos.mef:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.mef -D CATFORMAT LONEOS -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.mef -D CATFORMAT LONEOS -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.mef -D CATFORMAT LONEOS -D CATMODE MEF
	$(DVOCHECK) catdir.asca.loneos.mef asca

asca.elixir.mef:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	$(DVOCHECK) catdir.asca.elixir.mef asca

asca.pmtest.mef:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.mef -D CATFORMAT PMTEST -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.mef -D CATFORMAT PMTEST -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.mef -D CATFORMAT PMTEST -D CATMODE MEF
	$(DVOCHECK) catdir.asca.pmtest.mef asca

asca.panstarrs.mef:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.mef -D CATFORMAT PANSTARRS -D CATMODE MEF
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.mef -D CATFORMAT PANSTARRS -D CATMODE MEF
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.mef -D CATFORMAT PANSTARRS -D CATMODE MEF
	$(DVOCHECK) catdir.asca.panstarrs.mef asca

#######
asca.loneos.split:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.split -D CATFORMAT LONEOS -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.split -D CATFORMAT LONEOS -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.loneos.split -D CATFORMAT LONEOS -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.loneos.split asca

asca.elixir.split:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.elixir.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.elixir.split asca

asca.pmtest.split:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.split -D CATFORMAT PMTEST -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.split -D CATFORMAT PMTEST -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.pmtest.split -D CATFORMAT PMTEST -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.pmtest.split asca

asca.panstarrs.split:
	addstar asca/to_r20060121ut005757s32340.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.split -D CATFORMAT PANSTARRS -D CATMODE SPLIT
	addstar asca/to_r20060121ut015747s35940.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.split -D CATFORMAT PANSTARRS -D CATMODE SPLIT
	addstar asca/to_r20060121ut025738s39540.smp -p ASCA_RED -D CATDIR catdir.asca.panstarrs.split -D CATFORMAT PANSTARRS -D CATMODE SPLIT
	$(DVOCHECK) catdir.asca.panstarrs.split asca

# megacam images:
# add in both FITS and RAW versions of output data
# 'best' and 'ap25' are from Sextractor / imclean
# 'ps10' is from psphot direct

megacam: megacam.raw megacam.mef megacam.split

megacam.raw: megacam.loneos.raw megacam.elixir.raw megacam.pmtest.raw megacam.panstarrs.raw
megacam.mef: megacam.loneos.mef megacam.elixir.mef megacam.pmtest.mef megacam.panstarrs.mef
megacam.split: megacam.loneos.split megacam.elixir.split megacam.pmtest.split megacam.panstarrs.split

#######
megacam.loneos.raw:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.loneos.raw -D CATFORMAT LONEOS -D CATMODE RAW
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.loneos.raw -D CATFORMAT LONEOS -D CATMODE RAW
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.loneos.raw -D CATFORMAT LONEOS -D CATMODE RAW
	$(DVOCHECK) catdir.megacam.loneos.raw megacam

megacam.elixir.raw:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.elixir.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.elixir.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.elixir.raw -D CATFORMAT ELIXIR -D CATMODE RAW
	$(DVOCHECK) catdir.megacam.elixir.raw megacam

megacam.pmtest.raw:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.pmtest.raw -D CATFORMAT PMTEST -D CATMODE RAW
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.pmtest.raw -D CATFORMAT PMTEST -D CATMODE RAW
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.pmtest.raw -D CATFORMAT PMTEST -D CATMODE RAW
	$(DVOCHECK) catdir.megacam.pmtest.raw megacam

megacam.panstarrs.raw:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.panstarrs.raw -D CATFORMAT PANSTARRS -D CATMODE RAW
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.panstarrs.raw -D CATFORMAT PANSTARRS -D CATMODE RAW
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.panstarrs.raw -D CATFORMAT PANSTARRS -D CATMODE RAW
	$(DVOCHECK) catdir.megacam.panstarrs.raw megacam

#######
megacam.loneos.mef:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.loneos.mef -D CATFORMAT LONEOS -D CATMODE MEF
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.loneos.mef -D CATFORMAT LONEOS -D CATMODE MEF
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.loneos.mef -D CATFORMAT LONEOS -D CATMODE MEF
	$(DVOCHECK) catdir.megacam.loneos.mef megacam

megacam.elixir.mef:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.elixir.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.elixir.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.elixir.mef -D CATFORMAT ELIXIR -D CATMODE MEF
	$(DVOCHECK) catdir.megacam.elixir.mef megacam

megacam.pmtest.mef:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.pmtest.mef -D CATFORMAT PMTEST -D CATMODE MEF
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.pmtest.mef -D CATFORMAT PMTEST -D CATMODE MEF
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.pmtest.mef -D CATFORMAT PMTEST -D CATMODE MEF
	$(DVOCHECK) catdir.megacam.pmtest.mef megacam

megacam.panstarrs.mef:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.panstarrs.mef -D CATFORMAT PANSTARRS -D CATMODE MEF
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.panstarrs.mef -D CATFORMAT PANSTARRS -D CATMODE MEF
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.panstarrs.mef -D CATFORMAT PANSTARRS -D CATMODE MEF
	$(DVOCHECK) catdir.megacam.panstarrs.mef megacam

#######
megacam.loneos.split:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.loneos.split -D CATFORMAT LONEOS -D CATMODE SPLIT
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.loneos.split -D CATFORMAT LONEOS -D CATMODE SPLIT
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.loneos.split -D CATFORMAT LONEOS -D CATMODE SPLIT
	$(DVOCHECK) catdir.megacam.loneos.split megacam

megacam.elixir.split:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.elixir.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.elixir.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.elixir.split -D CATFORMAT ELIXIR -D CATMODE SPLIT
	$(DVOCHECK) catdir.megacam.elixir.split megacam

megacam.pmtest.split:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.pmtest.split -D CATFORMAT PMTEST -D CATMODE SPLIT
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.pmtest.split -D CATFORMAT PMTEST -D CATMODE SPLIT
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.pmtest.split -D CATFORMAT PMTEST -D CATMODE SPLIT
	$(DVOCHECK) catdir.megacam.pmtest.split megacam

megacam.panstarrs.split:
	addstar megacam/720357o13.best.smp -D CATDIR catdir.megacam.panstarrs.split -D CATFORMAT PANSTARRS -D CATMODE SPLIT
	addstar megacam/720357o13.ap25.smp -D CATDIR catdir.megacam.panstarrs.split -D CATFORMAT PANSTARRS -D CATMODE SPLIT
	addstar megacam/720357o13.ps10.cmp -D CATDIR catdir.megacam.panstarrs.split -D CATFORMAT PANSTARRS -D CATMODE SPLIT
	$(DVOCHECK) catdir.megacam.panstarrs.split megacam

#######

clean:
	rm -rf catdir.*
