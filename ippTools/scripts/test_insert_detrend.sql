-- create a fake detrun, add fake detProcessedImfile entries, etc

insert into 
       detRun (det_id, det_type, iteration, camera, workdir,   state, mode) 
       values (1,      'flat',           0, 'GPC1', 'testdir', 'run', 'master');

insert into 
       detProcessedImfile (det_id, class_id) 
       values (1, 'ccd00');

insert into 
       detProcessedImfile (det_id, class_id) 
       values (1, 'ccd01');

insert into 
       detResidImfile (det_id, iteration, class_id) 
       values (1, 0, 'ccd00');

insert into 
       detResidImfile (det_id, iteration, class_id) 
       values (1, 0, 'ccd01');

insert into 
       detNormalizedStatImfile (det_id, iteration, class_id) 
       values (1, 0, 'ccd01');

insert into 
       detNormalizedStatImfile (det_id, iteration, class_id) 
       values (1, 0, 'ccd00');

update detRun set iteration = 1 where det_id = 1;

insert into 
       detResidImfile (det_id, iteration, class_id) 
       values (1, 1, 'ccd00');

insert into 
       detResidImfile (det_id, iteration, class_id) 
       values (1, 1, 'ccd01');

insert into 
       detNormalizedStatImfile (det_id, iteration, class_id) 
       values (1, 1, 'ccd00');

insert into 
       detNormalizedStatImfile (det_id, iteration, class_id) 
       values (1, 1, 'ccd01');
