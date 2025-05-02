# include "fakeastro.h"

int main (int argc, char **argv) {

  ohana_gaussdev_init ();

  /* get configuration info, args */
  initialize (argc, argv);

  switch (FAKEASTRO_OP) {
    case OP_GALAXY:
      /* the object analysis is a separate process iterating over catalogs */
      fakeastro_galaxy ();  // generate fake stars following a galaxy model
      /* make_fakestars() -- generates FakeAstro_Stars with density,distance and uR,uD following galaxy model
	 make_fakeqsos()
	 sortStars()
	 make_subset()
	 save_fakestars()
	   fakestar_catalog()
	     insert_fakestar() -- converts suppied FakeAstro_Stars and populates average/secfilt/starpar structures
	   (fakestar_save_stars) [fakeastro_client / fakestar_load_stars]
       */
      exit (0);

    case OP_IMAGES:
      fakeastro_images ();
      /* fakeastro_images_region()
	   load_fake_stars()
	   make_fake_images()
	   make_fake_stars()
	   fit_fake_stars()
	   save_fake_stars()
	     match_fake_stars()
       */
      exit (0);

    case OP_2MASS:
      fakeastro_2mass ();
      /* make_2mass_measures()
       */
      exit (0);

    case OP_GAIA:
      fakeastro_gaia ();
      /* make_gaia_measures()
       */
      exit (0);

    default:
      fprintf (stderr, "impossible!\n");
      abort();
  }
  exit (1);
}

/* fakeastro can be used to generate a DVO database of fake observations.  

   top-level modes:

   -galaxy : generate a galaxy model. output is a DVO database with average.d and starpar.d tables populated
   -images : generate a set of observations (or just images?)

*/
