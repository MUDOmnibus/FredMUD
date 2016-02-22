#!/usr/bin/perl

&do_obj_file();

sub do_obj_file {
  local($/);

  open(OBJF,"../lib/big.obj") || die "$!\n";
  $chunk = <OBJF>;
  print length($chunk) , "\n";
  @a=split('#',$chunk);
  shift(@a);
  foreach $x (@a){
    $x =~ s/\n/ /g;
    @b=split('~',$x);
    @c=split(' ',$b[0]);
    $n = $c[0];
    @d=split(' ',$b[3]);
    if($d[0] == 9){
      printf "%5d %8x %s\n",$n, $d[2], $b[1];
    }
  }
}
