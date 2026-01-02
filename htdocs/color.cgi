#!/usr/bin/perl -Tw

use strict;
use CGI;

my $cgi = CGI->new();

print $cgi->header;
my $color = "blue";
$color = $cgi->param('color') if (defined $cgi->param('color') && $cgi->param('color') ne '');

print $cgi->start_html(-title => uc($color),
                       -BGCOLOR => $color);
print $cgi->h1("This is $color");
print $cgi->end_html;
