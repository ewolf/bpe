use lib "/home/wolf/perl/lib/perl5/x86_64-linux-gnu-thread-multi";

use BPE;
use Data::Dumper;

my $x = BPE::encode("updating tests. writing a test suite in the www/recipes/tests directory. want a program that will| load them all and run the tests for each of them, using SP.init, maybe make an SP.clear that will empty out the body. update tester so that there is something like doneTesting but not for the page, |maybe just returns the test results as an object.",2,'|');
print STDERR Data::Dumper->Dump([$x]);
my $cues = $x->{cues};
my $text = $x->{text};
print join( '', map { $cues->[$_] } @$text )."\n";

1;
