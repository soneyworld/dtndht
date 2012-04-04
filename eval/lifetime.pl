#!/usr/bin/perl
use strict;
use warnings;

my $PNG_OUTPUT_SIZE_X = 600;
my $PNG_OUTPUT_SIZE_Y = 400;

my $num_args = $#ARGV + 1;
if ($num_args < 1) {
  print "Invalid number of arguments!\n\nUsage: inputfile [inputfile ...]\n";
  exit;
}

# My own IP, which should be returned by the dht
my $correct_IP = "77.22.75.236";
# Key is the sha1 of the announced string, value is the string
my %announcement;

# Key is sha1 of the announced string, value is the duration
my %announcement_duration;
# Key is sha1 of the announced string, value is the start timestamp
my %announcement_timings;

my $announcement_output_file = "announcement_duration.csv";

foreach (@ARGV) {
	parseFile($_);
}

open(OUT, ">$announcement_output_file");
print OUT "EID;HASH;START;DURATION\n";
my %lines;
foreach ( keys %announcement_duration ) {
	my $eid = $announcement{"$_"};
	my $start = $announcement_timings{"$_"};
	my $duration = $announcement_duration{"$_"};
	if( exists $lines{$start} ) {
		$lines{$start} .= "$eid;$_;$start;$duration\n";
	} else {
		$lines{$start} = "$eid;$_;$start;$duration\n";
	}
}
my $time;
foreach $time (sort keys %lines) {
  print OUT "$lines{$time}";
}

close(OUT);


sub parseFile
{
	my $inputfile = $_[0];
	my $outputfile= $_[0] . ".csv";
	my $gnuplot_outputfile = $outputfile . ".plt";
	my $gnuplot_output_png = $outputfile . ".png";
	my $gnuplot_output_eps = $outputfile . ".eps";
	# Start of first Lookup
	my $startOfLookups = 0;
	my @lookupTimings;
	my @lookupDurations;
	my $lookupCounter = 0;

	# Time of the first valid hit
	my @firstHit;
	# Time of the second valid hit
	my @secondHit;
	# Time of the third valid hit
	my @thirdHit;
	# Number of invalid hits
	my @invalidValueCount;

	my $hitcounter = 0;

	open(IN, $inputfile);

	my $line = <IN>;

	while ($line) {
		# Zeile gehört zum Output der LIB
		if ($line =~ m/DHT-EVALUATION/) {
			if ($line =~ m/START_ANNOUNCE EID=/) {
				my $eid = extractEID($line);
				my $sha1 = extractHash($line);
				my $time = extractTime($line);
				$announcement{"$sha1"} = "$eid";
				$announcement_duration{"$sha1"} = "$time";
				$announcement_timings{"$sha1"} = "$time";
			}
			
			if ($line =~ m/DHT-EVALUATION ANNOUNCE_DONE_EVENT HASH=/) {
				my $sha1 = extractHash($line);
				my $time = extractTime($line);
				my $eid = $announcement{"$sha1"};
				my $duration = $time - $announcement_duration{"$sha1"};
				$announcement_duration{"$sha1"} = $duration;
			}
			# Start of Lookup for EID
			if ($line =~ m/DHT-EVALUATION DHT_SEARCH HASH=/) {
				my $time = extractTime($line);
				if( $startOfLookups == 0 ) {
					$startOfLookups = $time;
				}
				$lookupTimings[$lookupCounter] = $time;
				$invalidValueCount[$lookupCounter] = 0;
				$firstHit[$lookupCounter] = "";
				$secondHit[$lookupCounter] = "";
				$thirdHit[$lookupCounter] = "";
			}
			# End of Lookup for EID
			if ($line =~ m/DHT-EVALUATION SEARCH_DONE_EVENT HASH=/) {
				my $time = extractTime($line);
				$lookupDurations[$lookupCounter] = $time - $lookupTimings[$lookupCounter];
				$lookupCounter++;
				$hitcounter = 0;
			}
			if ($line =~ m/DHT-EVALUATION VALUES_FOUND_EVENT HASH=/) {
				my $time = extractTime($line);
				$line = <IN>;
				while ( $line =~ m/^---->/ ) {
					my @tokens = split(/\s+/, $line);
					if ($tokens[1] eq $correct_IP ) {
						$hitcounter++;
					} else {
						$invalidValueCount[$lookupCounter]++;
					}
					$line = <IN>;
				}
				if( $hitcounter == 1 ){
					$firstHit[$lookupCounter] = $time - $lookupTimings[$lookupCounter];
				}elsif ($hitcounter == 2 ) {
					$secondHit[$lookupCounter] = $time - $lookupTimings[$lookupCounter];
				}elsif ($hitcounter == 3 ) {
					$thirdHit[$lookupCounter] = $time - $lookupTimings[$lookupCounter];
				}
			}
		}
		$line = <IN>;
	}

	close(IN);

	open(OUT,">$outputfile");
	print OUT "Lookup;StartUntilFirstLookupCall;CompleteDuration;DurationToFirstCorrectValue;SecondCorrectValue;ThirdCorrectValue;InvalidValueCount;\n";
	my $i = 0;
	foreach ( @lookupDurations ) {
		my $lookup = $i + 1;
		my $duration = $_;
		my $first = $firstHit[$i];
		my $second = $secondHit[$i];
		my $third = $thirdHit[$i];
		my $invalid = $invalidValueCount[$i];
		my $start = $lookupTimings[$i] - $startOfLookups;
		print OUT "$lookup;$start;$duration;$first;$second;$third;$invalid;\n";
		$i++;
	}
	close(OUT);
	open( PLOT , ">$gnuplot_outputfile" );
	print PLOT "set terminal png truecolor font small size $PNG_OUTPUT_SIZE_X,$PNG_OUTPUT_SIZE_Y\nset output \'$gnuplot_output_png\'\n";
	print PLOT "set datafile separator \";\"\n";
	print PLOT "set xlabel 'duration until starting first lookup [sec]'\n";
	print PLOT "set ylabel 'duration until start of lookup [sec]'\n";
	print PLOT "set xrange [0:3600]\n";
	print PLOT "set yrange [0:300]\n";
#	print PLOT "set multiplot\n";
	print PLOT "plot '$outputfile' every::1::30 using 2:4 with linespoints title \"first correct answer\", \\\n";
	print PLOT "     '$outputfile' every::1::30 using 2:5 with linespoints title \"second correct answer\", \\\n";
	print PLOT "     '$outputfile' every::1::30 using 2:6 with linespoints title \"third correct answer\", \\\n";
	print PLOT "     '$outputfile' every::1::30 using 2:3 with histeps title \"complete duration\"\n";
#	print PLOT "set parametric\n";
#	print PLOT "const=1800\n";
#	print PLOT "plot const,t title \"30 minutes until start\"\n";
	print PLOT "set output \"$gnuplot_output_eps\"\n";
	print PLOT "set terminal postscript eps\n";
	print PLOT "replot\n";
	close(PLOT);
}# End parseFile

sub extractTime 
{
	my $time;
	my @tokens = split(/\s+/, $_[0]);
	foreach (@tokens) {
		if($_ =~ m/^TIME=/) {
			my @subtokens = split(/=/, $_);
			$time = $subtokens[1];
		}
	}
	return $time;
}# End extractTime

sub extractHash
{
	my $sha1;
	my @tokens = split(/\s+/, $_[0]);
	foreach (@tokens) {
		if( $_ =~ m/^HASH=/) {
			my @subtokens = split(/=/, $_);
			$sha1 = $subtokens[1];
		}
	}
	return $sha1;
}# End extractHash

sub extractEID
{
	my $eid;
	my @tokens = split(/\s+/, $_[0]);
	foreach (@tokens) {
		if( $_ =~ m/^EID=/) {
			my @subtokens = split(/=/, $_);
			$eid = $subtokens[1];
		}
	}
	return $eid;
}# End extractEID

