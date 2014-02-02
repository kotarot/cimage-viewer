#!/usr/bin/perl

# コンソール上で(無理矢理)動画再生する
#  ffmpeg -ss option で bmp 出力してCBmpViewer

use strict;
use warnings;
use Carp; 

my $ffmpeg_exe = 'ffmpeg';

my $TMPDIR = '/tmp/__out/';
# my $TMPDIR = 'R:\\__out/'              ;
# my $TMPDIR = '/mnt/hgfs/ramdisk/__out/';

# 動画の長さを採取
# x y dur[ms]
sub mpg_info
{
	my $file = shift;
	my ($x, $y, $dur) = (-1, -1, undef);
	open my $fh,  $ffmpeg_exe . ' -i "' . $file . '" 2>&1 |';
	my @info = <$fh>;
	close $fh;
#	print "@info ";
		
	foreach my $info(@info)
	{
		chomp $info;
		foreach my $line(split( /,/ , $info ))
		{
			if ($line =~ m/([0-9][0-9][0-9]+)x([0-9][0-9][0-9]+)/)
			{
				$x = $1;
				$y = $2;
			}
			if ($line =~ m/Duration:/)
			{
			    my $duration = $';
				my($time, undef) = split( /,/ , $duration );
				last if( $time =~ m#N/A# );
				
				my($t, $ms) = split( /\./ , $time );
				my($hh, $mm, $ss) = split( /\:/ , $t );
				$dur = ($hh * 3600 + $mm * 60 + $ss) * 1000 + $ms;
			}
		}
	}
	
	return($x, $y, $dur);
}

sub play
{
	my $file = shift;
#	print "$file\n";
	my $step = 1;
	my $scale = 48; # 駒のサイズ SSH越しだと48くらいが限界
	my $uniqueid = sprintf("%010d",time()).sprintf("%05d",$$);
	my $tmpdir =  $TMPDIR . '_' . $uniqueid;
	mkdir $TMPDIR;
	mkdir $tmpdir;
	unlink while(<$tmpdir . '/*.bmp'>);
	my ($x,  $y,  $d) = &mpg_info($file);
	return unless($d);
	for my $index( 0..$d/$step )
	{
		my $thumbp =
		   q#ffmpeg #
		 . q# -v quiet #
		 . q# -ss # . (sprintf "%d", $index * $step)
		 . q# -skip_frame nokey #
#		 . q# -skip_frame noref #
		 . qq# -i "$file" #
		 . qq# -vf scale=$scale:-1#
		 . qq# -f image2 "$tmpdir/$index.bmp" #;
		system($thumbp . ' >/dev/null 2>&1');
		die if(($? & 256) == 0);
		print "\x1b[2J"; # 画面クリア

		system(qq#cbmpviewer "$tmpdir/$index.bmp"#);
	}
	unlink while(<$tmpdir . '/*.bmp'>);
	rmdir $tmpdir;
	rmdir $TMPDIR;
}

#####################################
# ファイル指定がない場合は適当に探す
if(@ARGV)
{
	&play($_) foreach(@ARGV);
}
else
{
	 if(<*.ts>)
	 {
	 	&play(<*.ts>);
	 }
	 elsif(<*.mp4>)
	 {
	 	&play(<*.mp4>);
	 }
}

#####################################
__END__
