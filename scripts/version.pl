#!/usr/bin/perl -i.bak

$version = shift;

while (<>) {
  if (m!define JAZZ_VERSION!) {
    print "#define JAZZ_VERSION $version\n";
  }
  else {
    print;
  }
}


1;


