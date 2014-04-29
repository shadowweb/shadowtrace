#!/usr/bin/perl
use strict;
use warnings;
use integer;

my %options = (); 

sub usage
{
    print "-e <executable file> -i <tree file> -o <symbols file>\n";
    exit;
}

sub init
{
    use Getopt::Std;
    getopts("i:o:e:", \%options) or usage();
    usage() if not ($options{o} and $options{i} and $options{e});
}

my %symbols = ();

sub load_symbols
{
    my ($exe_file) = @_;
    print "Loading symbols from '$exe_file'...\n";
    if (-x $exe_file)
    {
        open FILE, "nm -C $exe_file |" or die "Failed to open pipe on \'nm -C $exe_file\'\n";
        while (<FILE>)
        {
            my $line = $_;
            chomp $line;
            if ($line =~ /([0-9a-fA-F]+)\s+\w\s+(.*)$/)
            {
                my $addr = $1;
                my $symbol = $2;
                $addr =~ s/^0+//;
                $symbols{$addr} = $symbol;
            }
        }
    }
}

my %func_addr = ();

sub load_input
{
    my ($input_file) = @_; 
    print "Loading '$input_file'...\n";
    open(INPUTFILE, "<$input_file") or die "Failed to open input file: '$input_file'\n";
    while (<INPUTFILE>)
    {
        my $line = $_;
        chomp $line;
        my @fields = split (/ /, $line);
        my $addr = $fields[1];
        if (exists $func_addr{$addr})
        {
            $func_addr{$addr}++;
        }
        else
        {
            $func_addr{$addr} = 1;
        }
    }
    close (INPUTFILE);
    return;
}

sub dump_output
{
    my ($output_file) = @_; 
    print "Writing to  '$output_file'...\n";
    open(OUTPUTFILE, ">$output_file") or die "Failed to open output file: '$output_file'\n";
    foreach my $addr (sort keys %func_addr)
    {
        my $stripped_addr = $addr;
        $stripped_addr =~ s/^0x//;
        my $symbol = (exists $symbols{$stripped_addr})? $symbols{$stripped_addr} : $stripped_addr;
        print OUTPUTFILE "$stripped_addr $func_addr{$addr} $symbol\n";
    }
    close(OUTPUTFILE);
}

init();

load_symbols($options{e});
load_input($options{i});
dump_output($options{o});
