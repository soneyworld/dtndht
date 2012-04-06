#!/usr/bin/perl
use strict;
use warnings;

my $PNG_OUTPUT_SIZE_X = 1400;
my $PNG_OUTPUT_SIZE_Y = 1000;

my $PNG_SUCCESS_OUTPUT_SIZE_X = 600;
my $PNG_SUCCESS_OUTPUT_SIZE_Y = 400;

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
my @average_First_Value;
my @average_Second_Value;
my @average_Third_Value;
my @average_First_Count;
my @average_Second_Count;
my @average_Third_Count;
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

sub printCSV
{
	open(CSV, ">lookup_concat.csv");
	my $i;
	print CSV "lookup;min1;max1;min2;max2;min3;max3;average1;average2;average3;novalue1;novalue2;novalue3;success1;success2;success3;\n";
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
		if (exists $average_First_Value[$i]){
			$average1 = $average_First_Value[$i] / $average_First_Count[$i];
		}
		if (exists $average_Second_Value[$i]){
			 $average2 = $average_Second_Value[$i] / $average_Second_Count[$i];
		}
		if (exists $average_Third_Value[$i]){
			$average3 = $average_Third_Value[$i] / $average_Third_Count[$i];
		}
		if (exists $minimum_First_Value_Lookup[$i]) {
			$min1 = $minimum_First_Value_Lookup[$i];
		}
		if (exists $minimum_Second_Value_Lookup[$i]) {
			$min2 = $minimum_Second_Value_Lookup[$i];
		}
		if (exists $minimum_Third_Value_Lookup[$i]) {
			$min3 = $minimum_Third_Value_Lookup[$i];
		}
		if (exists $maximum_First_Value_Lookup[$i]) {
			$max1 = $maximum_First_Value_Lookup[$i];
		}
		if (exists $maximum_Second_Value_Lookup[$i]) {
			$max2 = $maximum_Second_Value_Lookup[$i];
		}
		if (exists $maximum_Third_Value_Lookup[$i]) {
			$max3 = $maximum_Third_Value_Lookup[$i];
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
		print CSV "". $i+1 . ";$min1;$max1;$min2;$max2;$min3;$max3;$average1;$average2;$average3;$novalue1;$novalue2;$novalue3;$success1;$success2;$success3;\n";
	}
	close(CSV);
}


sub print_gnuplot_files
{
	open(FIRST, ">first_value_lookup.plt");
	print FIRST "set terminal png truecolor font \"Helvetica\" 10 size $PNG_OUTPUT_SIZE_X,$PNG_OUTPUT_SIZE_Y\nset output \'first_value_lookup.png\'\n";
	print FIRST "set datafile separator \";\"\n";
	print FIRST "set title \"box-and-whisker with median bar and whiskerbars\"\n";
	print FIRST "set ytics 60\n";
	print FIRST "set xtics 1\n";
	print FIRST "plot 'lookup_concat.csv' using 1:2:3:2:3 with candlesticks lt 3 lw 2 title 'First correct value' whiskerbars, \\\n";
    print FIRST " ''                 using 1:8:8:8:8 with candlesticks lt -1 lw 2 notitle, \\\n";
	print FIRST " ''                 using 1:4:5:4:5 with candlesticks lt 4 lw 2 title 'Second correct value' whiskerbars, \\\n";
    print FIRST " ''                 using 1:9:9:9:9 with candlesticks lt -2 lw 2 notitle, \\\n";
	print FIRST " ''                 using 1:6:7:6:7 with candlesticks lt 8 lw 2 title 'Third correct value' whiskerbars, \\\n";
    print FIRST " ''                 using 1:10:10:10:10 with candlesticks lt -3 lw 2 notitle \n";
	close(FIRST);

	open (SUCCESS, ">lookup_success.plt");
	print SUCCESS "set terminal png truecolor font \"Helvetica\" 10 size $PNG_SUCCESS_OUTPUT_SIZE_X,$PNG_SUCCESS_OUTPUT_SIZE_Y\nset output \'lookup_success.png\'\n";
	print SUCCESS "set datafile separator \";\"\n";
	print SUCCESS "set title \"Lookup success ratio\" font \"Helvetica,12\"\n";
	print SUCCESS "set xlabel 'Lookup Number'\n";
	print SUCCESS "set ylabel 'Success ratio [%]'\n";
	print SUCCESS "set xtics 2\n";
	print SUCCESS "set ytics 10\n";
	print SUCCESS "set yrange [0:100]\n";
	print SUCCESS "plot 'lookup_concat.csv' using 1:14 with lines title '1 correct value', \\\n";
	print SUCCESS " ''                 using 1:15 with lines title '2 correct values', \\\n";
	print SUCCESS " ''                 using 1:16 with lines title '3 correct values' \n";
	print SUCCESS "set output \"lookup_success.eps\"\n";
	print SUCCESS "set terminal postscript eps\n";
	print SUCCESS "replot\n";
	close(SUCCESS);
}

sub print_gnuplot_duration_summary
{
	my $gnuplot_file = $_[0] . ".plt";
	my $gnuplot_png_file = $_[0] . ".png";
	my $gnuplot_eps_file = $_[0] . ".eps";
	open( PLOT, ">$gnuplot_file.plt");
	print PLOT "set terminal png truecolor font \"Helvetica\" 10 size $PNG_OUTPUT_SIZE_X,$PNG_OUTPUT_SIZE_Y\nset output \'$gnuplot_png_file\'\n";
	print PLOT "set datafile separator \";\"\n";
	print PLOT "set title \"box-and-whisker with median bar and whiskerbars\"\n";
	print PLOT "set ytics 60\n";
	print PLOT "set xtics 1\n";
	print PLOT "plot 'lookup_concat.csv' using 1:2:3:2:3 with candlesticks lt 3 lw 2 title 'First correct value' whiskerbars, \\\n";
    print PLOT " ''                 using 1:8:8:8:8 with candlesticks lt -1 lw 2 notitle, \\\n";
	print PLOT " ''                 using 1:4:5:4:5 with candlesticks lt 4 lw 2 title 'Second correct value' whiskerbars, \\\n";
    print PLOT " ''                 using 1:9:9:9:9 with candlesticks lt -2 lw 2 notitle, \\\n";
	print PLOT " ''                 using 1:6:7:6:7 with candlesticks lt 8 lw 2 title 'Third correct value' whiskerbars, \\\n";
    print PLOT " ''                 using 1:10:10:10:10 with candlesticks lt -3 lw 2 notitle \n";
	close(PLOT);
}
