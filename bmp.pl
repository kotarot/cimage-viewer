#!/usr/bin/perl

use strict;
use warnings;
use Carp; 

my $ffmpeg_exe = 'ffmpeg';

sub view
{
    my $file = shift;
    my $TMPDIR = '/tmp/';
    my $tmp = $file;
    $tmp =~ tr#/#-#;
    open my $fh,  $ffmpeg_exe . ' -v quiet -i "' . $file . '" -f image2 ' . $TMPDIR . $tmp . '.bmp  2>&1 |';
    my @info = <$fh>;
    close $fh;
    # print "@info";
    system( 'cbmpviewer ' .  $TMPDIR . $tmp . '.bmp' );
}

&view($_) foreach(@ARGV);


