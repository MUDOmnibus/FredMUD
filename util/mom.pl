#!/usr/bin/perl

  open(FD,"quotes") || die "$!\n";
  while(<FD>){
    $len = length($_);
    print if(($len > 10)&&($len < 69));
  }
