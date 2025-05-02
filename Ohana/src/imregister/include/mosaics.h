
MosaicLayout *CreateCFH12K () {

  MosaicLayout *layout;

  ALLOCATE (layout, MosaicLayout, 1);

  layout[0].center.Nccd = 4;
  ALLOCATE (layout[0].center.ccd, int, layout[0].center.Nccd);
  layout[0].center.ccd[0] = 2;
  layout[0].center.ccd[1] = 3;
  layout[0].center.ccd[2] = 8;
  layout[0].center.ccd[3] = 9;

  layout[0].outer.Nccd = 8;
  ALLOCATE (layout[0].outer.ccd, int, layout[0].outer.Nccd);
  layout[0].outer.ccd[0] = 0;
  layout[0].outer.ccd[1] = 1;
  layout[0].outer.ccd[2] = 4;
  layout[0].outer.ccd[3] = 5;
  layout[0].outer.ccd[4] = 6;
  layout[0].outer.ccd[5] = 7;
  layout[0].outer.ccd[6] = 10;
  layout[0].outer.ccd[7] = 11;

  layout[0].top.Nccd = 6;
  ALLOCATE (layout[0].top.ccd, int, layout[0].top.Nccd);
  layout[0].top.ccd[0] = 0;
  layout[0].top.ccd[1] = 1;
  layout[0].top.ccd[2] = 2;
  layout[0].top.ccd[3] = 3;
  layout[0].top.ccd[4] = 4;
  layout[0].top.ccd[5] = 5;

  layout[0].bottom.Nccd = 6;
  ALLOCATE (layout[0].bottom.ccd, int, layout[0].bottom.Nccd);
  layout[0].bottom.ccd[0] = 6;
  layout[0].bottom.ccd[1] = 7;
  layout[0].bottom.ccd[2] = 8;
  layout[0].bottom.ccd[3] = 9;
  layout[0].bottom.ccd[4] = 10;
  layout[0].bottom.ccd[5] = 11;

  layout[0].left.Nccd = 6;
  ALLOCATE (layout[0].left.ccd, int, layout[0].left.Nccd);
  layout[0].left.ccd[0] = 0;
  layout[0].left.ccd[1] = 1;
  layout[0].left.ccd[2] = 2;
  layout[0].left.ccd[3] = 6;
  layout[0].left.ccd[4] = 7;
  layout[0].left.ccd[5] = 8;

  layout[0].right.Nccd = 6;
  ALLOCATE (layout[0].right.ccd, int, layout[0].right.Nccd);
  layout[0].right.ccd[0] = 3;
  layout[0].right.ccd[1] = 4;
  layout[0].right.ccd[2] = 5;
  layout[0].right.ccd[3] = 9;
  layout[0].right.ccd[4] = 10;
  layout[0].right.ccd[5] = 11;

  return (layout);
}
