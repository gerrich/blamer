#!/usr/bin/perl

use strict;
use warnings;

my %h = ();
while(<STDIN>){
  chomp;
  my @f = split /: /, $_;
  next if $#f != 1;
  next if exists $h{$f[0]} and $f[0] ne 'Run-status';
  $h{$f[0]} = $f[1];
}

if ($h{From} =~ s/\s\(uid (\d+)\)$//) {
  $h{uid} = $1;
}
print join("\t", @h{qw/Run-id Run-status From uid Date/});
print "\n";

