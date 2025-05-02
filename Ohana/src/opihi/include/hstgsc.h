
static double BigDecBounds[] = {0.0, 7.5, 15.0, 22.5, 30.0, 37.5, 45.0, 
				52.5, 60.0, 67.5, 75.0, 82.5, 90.0,
				0.0, -7.5, -15.0, -22.5, -30.0, -37.5, -45.0, 
				-52.5, -60.0, -67.5, -75.0, -82.5, -90.0};
static char *Dec2Sections[] = {"n0000", "n0730", "n1500", "n2230", "n3000", "n3730", "n4500", 
			       "n5230", "n6000", "n6730", "n7500", "n8230", "weirdness", 
			       "s0000", "s0730", "s1500", "s2230", "s3000", "s3730", "s4500", 
			       "s5230", "s6000", "s6730", "s7500", "s8230", "weirdness"};

static int NDecLines[] = {593, 584, 551, 530, 522, 465, 406, 362, 280, 198, 123, 24, 
			  0, 597, 578, 574, 577, 534, 499, 442, 376, 294, 212, 144, 48};

/** older data layout concepts 
static char *DecSections[] = {"N0000", "N0730", "N1500", "N2230", "N3000", "N3730", "N4500", 
			      "N5230", "N6000", "N6730", "N7500", "N8230", "weirdness", 
			      "S0000", "S0730", "S1500", "S2230", "S3000", "S3730", "S4500", 
			      "S5230", "S6000", "S6730", "S7500", "S8230", "weirdness"};

static char *disk[] = {"disk 1", "disk 1", "disk 1", "disk 1", "disk 1", "disk 1", "disk 1", 
		       "disk 1", "disk 1", "disk 1", "disk 1", "disk 1", "weirdness", 
		       "disk 1", "disk 2", "disk 2", "disk 2", "disk 2", "disk 2", "disk 2", 
		       "disk 2", "disk 2", "disk 2", "disk 2", "disk 2", "weirdness"};

static int NBigRASections [] = {48, 47, 45, 43, 40, 36, 32, 28, 21, 15, 9, 3, 3, 48, 47, 45, 43, 40, 36, 32, 28, 21, 15, 9, 3, 3};

**/
