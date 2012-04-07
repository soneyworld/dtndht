#!/usr/bin/perl
use strict;
use warnings;
use Switch;

my $PNG_OUTPUT_SIZE_X = 1200;
my $PNG_OUTPUT_SIZE_Y = 800;

my $PNG_SUCCESS_OUTPUT_SIZE_X = 600;
my $PNG_SUCCESS_OUTPUT_SIZE_Y = 400;

my $LOW_QUANTIL = 0.05;
my $HIGH_QUANTIL = 0.95;

my $num_args = $#ARGV + 1;
if ($num_args < 1) {
  print "Invalid number of arguments!\n\nUsage: lookup_contact.pl inputfile [inputfile ...]\n";
  exit;
}

# Less than 2% has got a result for a lookup, so the success ration should ignore this
my $MINIMUM_LOOKUP_COUNT = $num_args / 100;

my @minimum_First_Value_Lookup;
my @maximum_First_Value_Lookup;
my @minimum_Second_Value_Lookup;
my @maximum_Second_Value_Lookup;
my @minimum_Third_Value_Lookup;
my @maximum_Third_Value_Lookup;
my @no_Correct_Value;
my @no_Second_Correct_Value;
my @no_Third_Correct_Value;
my @invalid_Value;
my @average_First_Value;
my @average_Second_Value;
my @average_Third_Value;
my @average_First_Count;
my @average_Second_Count;
my @average_Third_Count;
my @timings_First_Value;
my @timings_Second_Value;
my @timings_Third_Value;
my @median_First_Value;
my @median_Second_Value;
my @median_Third_Value;
my @lookupCount;

my $maximum_Number_of_Lookups = 0;

foreach (@ARGV) {
	parseFile($_);
}

printCSV();

print_gnuplot_files();
exit;

sub parseFile
{
	my $inputfile = $_[0];
	my $lookupCounter = 0;
	open(IN, $inputfile);

	# Ingnoring first line
	my $line = <IN>;
	$line = <IN>;
	# Reading line by line
	while ($line) {
		my @tokens = split(/;/, $line);
		if(@tokens == 8 ) {
			if ($tokens[3] =~ m/^(\d+)$/ ) {
				addFirstValue($tokens[3], $lookupCounter);
			} else {
				addNoCorrectValue($lookupCounter);
			}
			if ($tokens[4] =~ m/^(\d+)$/ ) {
				addSecondValue($tokens[4], $lookupCounter);
			} else {
				addNoSecondCorrectValue($lookupCounter);
			}
			if ($tokens[5] =~ m/^(\d+)$/ ) {
				addThirdValue($tokens[5], $lookupCounter);
			} else {
				addNoThirdCorrectValue($lookupCounter);
			}
			if ($tokens[6] =~ m/^(\d+)$/ ) {
				addInvalidAnswer($lookupCounter, $tokens[6]);
			}
			if (exists $lookupCount[$lookupCounter]) {
				$lookupCount[$lookupCounter]++;
			} else {
				$lookupCount[$lookupCounter] = 1;
			}			
			$lookupCounter++;
		}
		$line = <IN>;
	}

	if($lookupCounter > $maximum_Number_of_Lookups) {
		$maximum_Number_of_Lookups = $lookupCounter;
	}

	close(IN);
}

sub addFirstValue
{
	my $first = $_[0];
	my $lookupCounter = $_[1];
	if( exists $minimum_First_Value_Lookup[$lookupCounter] ) {
		if($minimum_First_Value_Lookup[$lookupCounter] > $first) {
			$minimum_First_Value_Lookup[$lookupCounter] = $first;
		} 
	} else {
		$minimum_First_Value_Lookup[$lookupCounter] = $first;
	}
	if( exists $maximum_First_Value_Lookup[$lookupCounter] ) {
		if($maximum_First_Value_Lookup[$lookupCounter] < $first) {
			$maximum_First_Value_Lookup[$lookupCounter] = $first;
		}
	} else {
		$maximum_First_Value_Lookup[$lookupCounter] = $first;
	}
	if (exists $average_First_Value[$lookupCounter]) {
		$average_First_Value[$lookupCounter] += $first;
	} else {
		$average_First_Value[$lookupCounter] = $first;
	}
	if (exists $average_First_Count[$lookupCounter]) {
		$average_First_Count[$lookupCounter]++;
	} else {
		$average_First_Count[$lookupCounter] = 1;
	}
	addFirstTiming($lookupCounter,$first);
}

sub addSecondValue
{
	my $second = $_[0];
	my $lookupCounter = $_[1];
	if( exists $minimum_Second_Value_Lookup[$lookupCounter] ) {
		if($minimum_Second_Value_Lookup[$lookupCounter] > $second) {
			$minimum_Second_Value_Lookup[$lookupCounter] = $second;
		} 
	} else {
		$minimum_Second_Value_Lookup[$lookupCounter] = $second;
	}
	if( exists $maximum_Second_Value_Lookup[$lookupCounter] ) {
		if($maximum_Second_Value_Lookup[$lookupCounter] < $second) {
			$maximum_Second_Value_Lookup[$lookupCounter] = $second;
		}
	} else {
		$maximum_Second_Value_Lookup[$lookupCounter] = $second;
	}
	if (exists $average_Second_Value[$lookupCounter]) {
		$average_Second_Value[$lookupCounter] += $second;
	} else {
		$average_Second_Value[$lookupCounter] = $second;
	}
	if (exists $average_Second_Count[$lookupCounter]) {
		$average_Second_Count[$lookupCounter]++;
	} else {
		$average_Second_Count[$lookupCounter] = 1;
	}
	addSecondTiming($lookupCounter,$second);
}

sub addThirdValue
{
	my $third = $_[0];
	my $lookupCounter = $_[1];
	if( exists $minimum_Third_Value_Lookup[$lookupCounter] ) {
		if($minimum_Third_Value_Lookup[$lookupCounter] > $third) {
			$minimum_Third_Value_Lookup[$lookupCounter] = $third;
		} 
	} else {
		$minimum_Third_Value_Lookup[$lookupCounter] = $third;
	}
	if( exists $maximum_Third_Value_Lookup[$lookupCounter] ) {
		if($maximum_Third_Value_Lookup[$lookupCounter] < $third) {
			$maximum_Third_Value_Lookup[$lookupCounter] = $third;
		}
	} else {
		$maximum_Third_Value_Lookup[$lookupCounter] = $third;
	}
	if (exists $average_Third_Value[$lookupCounter]) {
		$average_Third_Value[$lookupCounter] += $third;
	} else {
		$average_Third_Value[$lookupCounter] = $third;
	}
	if (exists $average_Third_Count[$lookupCounter]) {
		$average_Third_Count[$lookupCounter]++;
	} else {
		$average_Third_Count[$lookupCounter] = 1;
	}
	addThirdTiming($lookupCounter,$third);
}

sub addNoCorrectValue
{
	my $lookupCounter = $_[0];
	if( exists $no_Correct_Value[$lookupCounter] ) {
		$no_Correct_Value[$lookupCounter]++;
	} else {
		$no_Correct_Value[$lookupCounter] = 1;
	}
}

sub addNoSecondCorrectValue
{
	my $lookupCounter = $_[0];
	if( exists $no_Second_Correct_Value[$lookupCounter] ) {
		$no_Second_Correct_Value[$lookupCounter]++;
	} else {
		$no_Second_Correct_Value[$lookupCounter] = 1;
	}
}

sub addNoThirdCorrectValue
{
	my $lookupCounter = $_[0];
	if( exists $no_Third_Correct_Value[$lookupCounter] ) {
		$no_Third_Correct_Value[$lookupCounter]++;
	} else {
		$no_Third_Correct_Value[$lookupCounter] = 1;
	}
}

sub addInvalidAnswer
{
	my $lookupCounter = $_[0];
	my $count = $_[1];
	if( exists $invalid_Value[$lookupCounter]) {
		$invalid_Value[$lookupCounter] += $count;
	} else {
		$invalid_Value[$lookupCounter] = $count;
	}
}

sub addFirstTiming
{
	my $lookupCounter = $_[0];
	my $timing = $_[1];
	if( exists $timings_First_Value[$lookupCounter]) {
		$timings_First_Value[$lookupCounter] .= ";$timing";
	} else {
		$timings_First_Value[$lookupCounter] = "$timing";
	}
}

sub addSecondTiming
{
	my $lookupCounter = $_[0];
	my $timing = $_[1];
	if( exists $timings_Second_Value[$lookupCounter]) {
		$timings_Second_Value[$lookupCounter] .= ";$timing";
	} else {
		$timings_Second_Value[$lookupCounter] = "$timing";
	}
}

sub addThirdTiming
{
	my $lookupCounter = $_[0];
	my $timing = $_[1];
	if( exists $timings_Third_Value[$lookupCounter]) {
		$timings_Third_Value[$lookupCounter] .= ";$timing";
	} else {
		$timings_Third_Value[$lookupCounter] = "$timing";
	}
}

sub printCSV
{
	open(CSV, ">lookup_concat.csv");
	my $i;
	print CSV "lookup;min1;min2;min3;max1;max2;max3;average1;average2;average3;novalue1;novalue2;novalue3;success1;success2;success3;invalidCount;median1;median2;median3;low1;low2;low3;high1;high2;high3;\n";
	for($i = 0; $i < $maximum_Number_of_Lookups;$i++) {
		my $average1 = "";
		my $average2 = "";
		my $average3 = "";
		my $min1 = "";
		my $min2 = "";
		my $min3 = "";
		my $max1 = "";
		my $max2 = "";
		my $max3 = "";
		my $novalue1 = "";
		my $novalue2 = "";
		my $novalue3 = "";
		my $success1 = 0;
		my $success2 = 0;
		my $success3 = 0;
		my $invalid = "";
		my $median1 = "";
		my $median2 = "";
		my $median3 = "";
		my $low1 = "";
		my $low2 = "";
		my $low3 = "";
		my $high1 = "";
		my $high2 = "";
		my $high3 = "";
		if (exists $invalid_Value[$i]) {
			$invalid = int($invalid_Value[$i]/$lookupCount[$i]);
		}
		if (exists $no_Correct_Value[$i]) {
			$novalue1 = $no_Correct_Value[$i];
			if($lookupCount[$i]>$MINIMUM_LOOKUP_COUNT) {
				$success1 = 100 - ($novalue1*100)/$lookupCount[$i];
			} else {
				$success1 = "";
			}
		}
		if (exists $no_Second_Correct_Value[$i]) {
			$novalue2 = $no_Second_Correct_Value[$i];
			if($lookupCount[$i]>$MINIMUM_LOOKUP_COUNT) {
				$success2 = 100 - ($novalue2*100)/$lookupCount[$i];
			} else {
				$success2 = "";
			}
		}
		if (exists $no_Third_Correct_Value[$i]) {
			$novalue3 = $no_Third_Correct_Value[$i];
			if($lookupCount[$i]>$MINIMUM_LOOKUP_COUNT) {
				$success3 = 100 - ($novalue3*100)/$lookupCount[$i];
			} else {
				$success3 = "";
			}
		}
		if (exists $timings_First_Value[$i]) {
			my @values = split(/;/, $timings_First_Value[$i]);
			$min1 = calculateMin(@values);
			$max1 = calculateMax(@values);
			$average1 = calculateAverage(@values);
			$median1 = calculateMedian(@values);
			$low1 = calculateLowQuantil(@values);
			$high1 = calculateHighQuantil(@values);
		}
		if (exists $timings_Second_Value[$i]) {
			my @values = split(/;/, $timings_Second_Value[$i]);
			$min2 = calculateMin(@values);
			$max2 = calculateMax(@values);
			$average2 = calculateAverage(@values);
			$median2 = calculateMedian(@values);
			$low2 = calculateLowQuantil(@values);
			$high2 = calculateHighQuantil(@values);
		}
		if (exists $timings_Third_Value[$i]) {
			my @values = split(/;/, $timings_Third_Value[$i]);
			$min3 = calculateMin(@values);
			$max3 = calculateMax(@values);
			$average3 = calculateAverage(@values);
			$median3 = calculateMedian(@values);
			$low3 = calculateLowQuantil(@values);
			$high3 = calculateHighQuantil(@values);
		}
		print CSV "". $i+1 . ";$min1;$min2;$min3;$max1;$max2;$max3;$average1;$average2;$average3;$novalue1;$novalue2;$novalue3;$success1;$success2;$success3;$invalid;$median1;$median2;$median3;$low1;$low2;$low3;$high1;$high1;$high3;\n";
	}
	close(CSV);
}

sub calculateMedian
{
	my $length = @_;
	@_ =  sort {$a <=> $b} (@_);
	my $median;
	if($length%2 == 0 ) {
		$median = ($_[$length/2] + $_[($length/2)-1])/2;
	}else{
		$median = $_[int($length/2)];
	}
	return $median;
}

sub calculateLowQuantil
{
	my $n = @_;
	my @array = sort {$a <=> $b} (@_);
	my $i = int($n*$LOW_QUANTIL);
	my $min = calculateMin(@array);
	my $median = calculateMedian(@array);
	if($min > $array[$i]) {
		print "$i: min=$min low=$array[$i]\n";
	}
	if($median < $array[$i]) {
		print "$i: median=$median low=$array[$i]\n";
		print "@array\n";
	}
	return $array[$i];
}

sub calculateHighQuantil
{
	my $n = @_;
	my @array = sort {$a <=> $b} (@_);
	my $i = int($n*$HIGH_QUANTIL);
	my $max = calculateMax(@array);
	my $median = calculateMedian(@array);
	if ($max < $array[$i]) {
		print "$i: max=$max high=$array[$i]\n";
	}
	if ($median > $array[$i]) {
		print "$i: median=$max high=$array[$i]\n";
		print "@array\n";
	}
	return $array[$i];
}

sub calculateMin
{
	my $min = $_[0];
	foreach (@_){
		if( $min > $_ ) {
			$min = $_;
		}
	}
	return $min;
}

sub calculateMax
{
	my $max = $_[0];
	foreach (@_){
		if( $max < $_ ) {
			$max = $_;
		}
	}
	return $max;
}

sub calculateAverage
{
	my $average;
	my $length = @_;
	foreach (@_) {
		$average += $_;
	}
	$average /= $length;
	return $average;
}

sub print_gnuplot_files
{
	print_gnuplot_duration_summary("first_value_lookup",1);
	print_gnuplot_duration_summary("second_value_lookup",2);
	print_gnuplot_duration_summary("third_value_lookup",3);

	open (SUCCESS, ">lookup_success.plt");
	print SUCCESS "set terminal png truecolor font \"Helvetica\" 10 size $PNG_SUCCESS_OUTPUT_SIZE_X,$PNG_SUCCESS_OUTPUT_SIZE_Y\nset output \'lookup_success.png\'\n";
	print SUCCESS "set datafile separator \";\"\n";
	print SUCCESS "set title \"Lookup success ratio\" font \"Helvetica,12\"\n";
	print SUCCESS "set xlabel 'Lookup Number'\n";
	print SUCCESS "set ylabel 'Success ratio [%]'\n";
	print SUCCESS "set xtics 2\n";
	print SUCCESS "set ytics 10\n";
	print SUCCESS "set yrange [0:100]\n";
	print SUCCESS "set autoscale y2\n";
	print SUCCESS "set y2label 'Number of false values'\n";
	print SUCCESS "plot 'lookup_concat.csv' using 1:14 axis x1y1 with lines title 'first correct value', \\\n";
	print SUCCESS " ''                 using 1:15 axis x1y1 with lines title 'second correct value', \\\n";
	print SUCCESS " ''                 using 1:16 axis x1y1 with lines title 'third correct value', \\\n";
	print SUCCESS " ''                 using 1:17 axis x1y2 with histeps title 'invalid values per lookup [truncated]' \n";
	print SUCCESS "set output \"lookup_success.eps\"\n";
	print SUCCESS "set terminal postscript enhanced color font \"Helvetica\" 10\n";
	print SUCCESS "replot\n";
	close(SUCCESS);
}

sub print_gnuplot_duration_summary
{
	my $gnuplot_file = $_[0] . ".plt";
	my $gnuplot_png_file = $_[0] . ".png";
	my $gnuplot_eps_file = $_[0] . ".eps";
	my $correctValue = $_[1];
	my $min = $correctValue + 1;
	my $max = $correctValue + 4;
	my $lowquantil = $correctValue + 20;
	my $highquantil = $correctValue + 23;
	my $average = $correctValue + 7;
	my $median = $correctValue + 17;
	my $color = getColor($correctValue);
	my $ratio = $correctValue + 13;
	open( PLOT, ">$gnuplot_file");
	print PLOT "set terminal png truecolor font \"Helvetica\" 10 size $PNG_OUTPUT_SIZE_X,$PNG_OUTPUT_SIZE_Y\nset output \'$gnuplot_png_file\'\n";
	print PLOT "set datafile separator \";\"\n";
	print PLOT "set title \"Duration until $correctValue correct value was found\" font \"Helvetica,12\"\n";
	print PLOT "set ytics 60\n";
	print PLOT "set xtics 1,1\n";
	print PLOT "set y2range [0:100]\n";
	print PLOT "set yrange [0:360]\n";
	print PLOT "set xrange [0:42]\n";
	print PLOT "set y2tics 10\n";
	print PLOT "set xlabel 'Lookup Number'\n";
	print PLOT "set ylabel 'Duration until start of lookup [sec]'\n";
	print PLOT "set y2label 'Success ratio [%]'\n";
	print PLOT "set ytics nomirror\n";
	print PLOT "set boxwidth 0.4\n";
	print PLOT "plot 'lookup_concat.csv' using 1:$lowquantil:$min:$max:$highquantil axis x1y1 with candlesticks lt $color lw 1 notitle whiskerbars, \\\n";
#    print PLOT " ''                 using 1:$average:$average:$average:$average axis x1y1 with candlesticks lt -1 lw 2 notitle, \\\n";
    print PLOT " ''                 using 1:$median:$median:$median:$median axis x1y1 with candlesticks lt -1 lw 2 notitle, \\\n";
	print PLOT " ''                 using 1:$ratio axis x1y2 with steps lt 1 title 'success ratio' \n";
	print PLOT "set output \"$gnuplot_eps_file\"\n";
	print PLOT "set terminal postscript enhanced color font \"Helvetica\" 10\n";
	print PLOT "replot\n";
	close(PLOT);
}

sub getColor
{
	switch ($_[0]) {
		case 1 {return 3}
		case 2 {return 4}
		case 3 {return 5}
		else   {return $_[0] * -1}
	}
}

