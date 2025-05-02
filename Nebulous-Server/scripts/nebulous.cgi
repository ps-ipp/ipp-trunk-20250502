#!/usr/bin/perl

# Copyright (C) 2004  Joshua Hoblitt
#
# $Id
 
use strict;
use warnings FATAL => qw( all );

use SOAP::Transport::HTTP;
use Nebulous::Server;
   
SOAP::Transport::HTTP::CGI
    -> dispatch_to( 'Nebulous::Server' )
    -> handle;
