Sparrowhawk Documentation
====================

Requirements
-----------

As noted in the README text file distributed with this package, in order to
build Sparrowhawk you must have installed the following packages:

  - OpenFst 1.5.4 or higher (<http://www.openfst.org>)
  - Thrax 1.2.2 or higher (<http://www.openfst.org/twiki/bin/view/GRM/Thrax>)
  - re2 (<https://github.com/google/re2>)
  - protobuf (<http://protobuf.googlecode.com/files/protobuf-2.5.0.tar.gz> ---
    see e.g. <http://jugnu-life.blogspot.com/2013/09/install-protobuf-25-on-ubuntu.html>)

General flow
-----------

Sparrowhawk is a pared-down version of Kestrel, the Google-internal TTS text
normalization system reported in Ebden and Sproat (2014). The flow of
information in Sparrowhawk more or less follows that of Kestrel, and since that
is described in detail in the cited paper we merely highlight the flow here.

Text comes in and is split into sentences using the sentence boundary detector
discussed below. Each sentence is then passed to the tokenizer and classifier,
which classifies tokens and simultaneously parses them into separate tokens. The
tokens are then parsed into an internal data structure (a protocol buffer Token
message), and collected into an utterance structure.

The utterance is then passed to the verbalizer. All of the "interesting" tokens
&mdash; things like numbers, money expression, times and so forth &mdash; are
treated by the verbalizer grammar rules and converted into words. As we shall
see below, the verbalizer allows for various possible orderings of the parsed
pieces of a token so that one can, e.g., verbalize the hour before the minutes
("three ten") or the minutes before the hour ("ten past three").

Text normalization grammars
-------------------------

All of the grammars for Sparrowhawk are written in the Thrax finite-state
grammar development environment. It is therefore assumed that users will be
familiar with weighted finite-state transducers, notions like composition,
projection and so forth, and how to develop grammars in Thrax. Documentation
for Thrax can be found at: <http://www.openfst.org/twiki/bin/view/GRM/Thrax>.

There are two main components of Sparrowhawk, namely:

1. The tokenizer/classifier, and
2. The verbalizer

The two components communicate with each other via a *protocol buffer*
specification defined in
<https://github.com/google/sparrowhawk/tree/master/src/proto>.  More
specifically, each input sentence is tokenized and each token is classified: the
tokenization and classification grammar is actually written so that the
tokenization and classification occur in parallel; more on that below. The
output is in the form of a serialization of tokens so that we have something
like

<pre>
tokens { tok1 } tokens { tok2 } ... tokens { tokn }
</pre>

representing a serialization of *token* protocol buffers.  This is then parsed
into an internal representation of the protocol buffer, and then reserialized in
various orders to be passed to the verbalizer.

The reordering makes it easier to handle cases like currency expressions where
at the tokenizer level an example like *$3.50* has the currency type (*dollars*)
before the number, whereas one would want to verbalize it after the number.
This kind of reordering is not easy to do in finite-state transducers, and so
the verbalizer allows both orderings:

<pre>
money { currency: "usd" amount { integer_part: "3" fractional_part: "50"} }
money { amount { integer_part: "3" fractional_part: "50"} currency: "usd" }
</pre>

Depending upon how one writes one's verbalizer grammar, these can be verbalized
in either or even both orders (though for most languages in this case one would
generally want the currency verbalized after the amount).

Note that the verbalizer only looks at tokens that belong to *semiotic classes*,
things like numbers, dates, times, currency amounts, measure expressions, and
so forth. It does not consider plain tokens.

We now turn to a deep-dive on the tokenizer/classifier and verbalizer grammars.

### Tokenizer/classifier grammars

Under the "grammars" subdirectory we provide a simple example toy grammar for
English that handles as semiotic classes cardinal numbers, simple times, simple
money expressions for three major currencies, simple dates and simple measure
expressions for three measure expressions. In this section we walk through the
example of monetary expressions in "grammars/en&#x005f;toy/classify/money.grm", and
describe how the whole tokenization system works.

The money grammar takes expressions of the form "$&lt;number&gt;" or
"$&lt;number&gt;.&lt;digit&gt;&lt;digit&gt;" (similarly for pounds and euros),
and converts them into a serialized version of the Money message (see
"src/proto/semiotic&#x005f;classes.proto"). Thus for example "$2.50" would become:

<pre>
money { currency: "usd" amount { integer_part: "2" fractional_part: "50"} }
</pre>

whereas "$200" would become:

<pre>
money { currency: "usd" amount { integer_part: "200"} }
</pre>

A whole sentence would be parsed into a sequence of tokens as noted above, where
each token is either a semiotic class, punctuation or an ordinary token. Thus
"he gave me $2." becomes:

<pre>
tokens { name: "he" }
tokens { name: "gave" }
tokens { name: "me" }
tokens { money { currency: "usd" amount { integer_part: "2"} } }
tokens { name: "." pause_length: PAUSE_LONG phrase_break: true type: PUNCT }
</pre>

The whole sequence is then deserialized in Sparrowhawk into a sequence of Token
messages, and an Utterance structure is created with the tokens as
dependents. For this to work, it is very important that each token in the output
of the tokenizer/classifier grammar be delimited by "token { ... }", and that of
course each of the token types match the pattern expected for the token proto so
that the protocol buffer deserializer can parse them. In particular ordinary
tokens and punctuation symbols should have "name" entries where the argument is
a double-quote-delimited string. For the semiotic classes, one should refer to
the definitions in "semiotic&#x005f;classes.proto" to see what is expected. Note in
particular that while most fields are defined as strings, for historical reasons
the numeric fields in the Time class are defined as integers, which means that
one should not use quotes around the hours, minutes and seconds fields: <pre>
time { hours: 3 minutes: 30} </pre>

For the numeric fields, such as currency amounts, the present grammar does not
allow the normal thousands delimiters (e.g. "3,000"). Thus "$3000" will work but
"$3,000" will not be correctly handled. We leave it as an exercise for the
reader to extend the grammar to handle such cases.

### Verbalizer grammars

In the verbalizer, each token in an utterance is examined one by one from left
to right. Ordinary words are left alone and punctuation tokens are examined and if
the token is defined as a phrase-break (see the example of a full sentence
above), it is verbalized as "sil" (for "silence").

Semiotic classes have a more complex treatment.  The message is serialized into
an FST, which is then composed with the verbalizer rules defined in the
verbalizer parameter file (see below).  However in many cases it is desirable to
serialize in various orders. Consider the case of money, where the input
ordering after parsing is as above, e.g.:

<pre>
money { currency: "usd" amount { integer_part: "200"} }
</pre>

This reflects the written order ("$200") but we actually want to *read* this
expression in the order number, then currency expression: "two hundred
dollars".

To handle this, Sparrowhawk serializes fields in all possible orders. It is then
up to the grammar writer to write the verbalization grammars in such a way that
one allows the field orders that are needed for the language. This lattice is
then composed internally to Sparrowhawk with the verbalizer grammars.

For English for example one might serialize a time as

<pre>
time { hours: 3 minutes: 10 }
</pre>

which would be convenient for reading it as "three ten". Or one might
serialize it as

<pre>
time { minutes: 10 hours: 3 }
</pre>

which would be convenient for verbalizing it as "ten past three".

Let us take money again as an example of how to write verbalization rules. The
grammar itself is distributed in "grammars/en&#x005f;toy/verbalize/money.grm". This is
one of the more complicated verbalization grammars, so understanding this one
will be useful for understanding the other simpler grammars.

We start with a case of a simple full dollar amount such as the following, which
has been serialized in the appropriate order.

<pre>
money { amount { integer_part: "3" } currency: "usd" }
</pre>

The money grammar depends upon a couple of resources. First, an FST model of
number names that can transduce numbers (up to 15 digits long) into their
readings of cardinal number names. An equivalent FST for ordinals is also
included. These FSTs were trained from a small amount of labeled English data using
the algorithm reported in Gorman and Sproat (2016).

Second, a list of monetary words, both for major and minor currencies
("money.tsv"):

<pre>
usd_maj	dollars
usd_min	cents
gbp_maj	pounds
gbp_min	pence
eur_maj	euros
eur_min	cents
</pre>

Note that the forms are plural: we singularize them in the grammar just in case
the preceding context is just the word "one".

For whole currencies (no cents or pence) like the expression above the mapping
is fairly simple. One gets rid of the markup, verbalizes the number as a
cardinal number, inserts "&#x005f;maj" after the ISO currency code, and then composes
that with the money expressions above. Thus

<pre>
money { amount { integer_part: "3" } currency: "usd" }
</pre>

would become "three dollars". The singularization rule would apply just in case
the preceding context is just the word "one" so that

<pre>
money { amount { integer_part: "1" } currency: "usd" }
</pre>

would become "one dollar".

This works for the case of whole dollar (pound, euro ...) amounts, but if there
is also a fractional currency things are a little more complicated:

<pre>
money { amount { integer_part: "2" fractional_part: "50"} currency: "usd" }
</pre>

Here (at least in one way of reading currency amounts), we would want to read
this as "two dollars and fifty cents". The minor currency ("cents") must match
with the major currency ("dollars"). The easiest way to handle this would be if
we could arrange for the input to verbalization to look something like the
following:

<pre>
money { amount { integer_part: "2" } currency: "usd_maj" { fractional_part: "50"} currency: "usd_min" }
</pre>

Since such *copying* of structure is expensive in FSTs, we do this in code. That
is, in addition to passing the structure

<pre>
money { amount { integer_part: "2" fractional_part: "50"} currency: "usd" }
</pre>

(and permutations) we also pass

<pre>
money { amount { integer_part: "2" fractional_part: "50"} currency: "usd" }
money { amount { integer_part: "2" fractional_part: "50"} currency: "usd" }
</pre>

as part of the lattice, and then we have the verbalizer
operate on the two copies separately. With the first copy it deletes the
fractional part, inserts

<pre>
&#x005f;maj
</pre>

after the currency designator, and verbalizes
the result. For the second copy it deletes the integer part, inserts

<pre>
&#x005f;min
</pre>

after the currency designator, and verbalizes the result. The rules also insert
"and" between the two halves.  This is what the rather verbose "money&#x005f;all" rule
in the grammar does.

The verbalizer also needs to know what kinds of patterns to copy. This is
handled by the REDUP rule. If an input matches REDUP, then Sparrowhawk will copy
the region in code (by concatenating the FST with a copy of itself) and then
passes that to the verbalizer along with the uncopied original lattice.

One fiddly point is that the serializer serializes messages in a particular way,
inserting spaces around fields and curly braces. It is important that the
verbalizer grammars match against what the serializer produces. To err on the
safe side, in the verbalizer grammars we actually allow zero or more spaces
around these elements, so that it will match whatever the serializer produces.

Sometimes one does not want orders of fields to be rearranged. In English, dates
can be read with the month first ("January third twenty ten") or the day first
("the third of January twenty ten"). If one writes "Jan. 3, 2010", one expects
the system to *read* it in the same order as "January third twenty ten", not as
"the third of January twenty ten". Similarly if one writes "3 Jan., 2010" one
expects the system to read it in that order.  But given that the verbalizer
needs to be set up to read both

<pre>
date { day: "3" month: "January" year: "2010" }
</pre>

and

<pre>
date { month: "January" day: "3" year: "2010" }
</pre>

there is no guarantee that one will get the right ordering in the output. That
is where the tag "preserve&#x005f;order: true" comes in handy. If we arrange for the
output of the tokenizer/classifier to be

<pre>
date { day: "3" month: "January" year: "2010" preserve_order: true }
</pre>

then Sparrowhawk will record the ordering of the fields when it parses the
output of the tokenizer/classifier, and will not rearrange them during the
verbalizer.  The information on the order is preserved by the system inserting a
set of repeated "field&#x005f;order:" fields, which record the order in which the input
fields came. In effect the above would become:

<pre>
date { day: "3" month: "January" year: "2010" preserve_order: true
         field_order: "day" field_order: "month" field_order: "year"}
</pre>

Note that "field&#x005f;order" should *not* be explicitly set in the grammars.

Then only one order will be sent to the verbalizer. Note that the verbalizer
must expect the "field&#x005f;order" and "preserve&#x005f;order" tags in the input
(Sparrowhawk does not delete them prior to passing them to the verbalizer). This
is easily dealt with by making sure that the verbalizer rule deletes these
fields.

See "date.grm" in the classify and verbalize directories for an example of how
to use the "preserve&#x005f;order" functionality.

If a verbalization of a semiotic class fails for some reason, we need a
backoff. The backoff in Sparrowhawk is to retreat to the original token
(e.g. "3:30") and treat it as a verbatim element, indicated thus:

<pre>
verbatim: "3:30"
</pre>

We therefore have a grammar ("verbatim.grm") to deal with this. It simply reads
the token as a sequence of characters:

<pre>
3_character :_character 3_character 0_character
</pre>

### Verbalizer grammars: new serialization format (Sparrowhawk 1.0 and above)

With Sparrowhawk 1.0, we introduce a simpler format for verbalizer
grammars. The upside of this is that it makes writing the verbalizer grammars
quite a bit simpler. The downside is that it requires a serialization
specification proto instance (see below). This new format has no relevance to
the classifier grammars, which should be written as described above in any case.

The main salient differences between the previous format and the new
serialization format are first that the representation that is passed by the
serialization to the verbalizer is more compact. Instead of

<pre>
money { amount { integer_part: "3" } currency: "usd" }
</pre>

what gets passed is

<pre>
money|integer_part:3|currency:usd|
</pre>

For both major and minor currencies the verbalizer sees, e.g.:

<pre>
money|integer_part:3|currency:usd|fractional_part:50|currency:usd|
</pre>

The second major difference is that a REDUP rule is no longer needed. Rather the
serialization, and possible copying of elements is done in code, controlled by
the serialization specification, itself an ASCII protocol buffer representation
that is referenced by an additional optional specification in the Sparrowhawk
configuration file. An example is given in
"verbalizer&#x005f;serialization&#x005f;spec.ascii&#x005f;proto". This specifies
the serialization possibilities for the different classes. For money, the
specification:

<pre>
class_spec {
  semiotic_class: "money"
  style_spec {
    record_spec {
      field_path: "money.amount.integer_part"
      suffix_spec {
        field_path: "money.currency"
      }
    }
    record_spec {
      field_path: "money.amount.fractional_part"
      suffix_spec {
        field_path: "money.currency"
      }
    }
  }
}
</pre>

means that the integer part of the money expression and the fractional part are
verbalized in that order, and the repetition of the "money.currency" field has
the effect of duplicating the expression for the currency itself. Again, the
verbalizer grammar is responsible for determining that the first instance would
be read as the major currency expression, and the second as the minor currency
expression.

The protocol buffer definition of the serialization specification is found in
"src/proto/serialization&#x005f;spec.proto", which is also documented with
comments on the functions of the various fields.

The parallel English toy grammar in the new serializer format can be found in
"grammars/en&#x005f;toy/verbalize&#x005f;serialization".

### Sentence boundary detection

Sparrowhawk provides some simple support for sentence boundary detection. One
can specify a regular expression for characters that should count as
end-of-sentence delimiters, thus:

<pre>
sentence_boundary_regexp: "[\\.:!\\?] "
</pre>

One can also override these in particular cases by providing a path to a file
that specifies a list of cases where one should not treat a punctuation symbol
as punctuation. E.g.:

<pre>
Mr.
Mrs.
Dr.
St.
</pre>

### Parameter files

Three parameter files are required by Sparrowhawk. The first is the main
sparrowhawk configuration file, which lists the location of the tokenizer
configuration file, the verbalizer configuration file, the sentence boundary
regular expression and the sentence boundary exceptions, if any:

<pre>
tokenizer_grammar:  "tokenizer.ascii_proto"

verbalizer_grammar:  "verbalizer.ascii_proto"

sentence_boundary_regexp: "[\\.:!\\?] "

sentence_boundary_exceptions_file: "sentence_boundary_exceptions.txt"
</pre>

The tokenizer configuration file lists the location of the compiled far for the
tokenizer/classifier grammar, an arbitrary name for the grammar, and a list of
rules, which takes the form:

<pre>
rules { main: "RULE1" }
rules { main: "RULE2" }
rules { main: "RULE3" }
...
</pre>
The rules are applied in the order given.

For example:

<pre>
grammar_file: "en_toy/classify/tokenize_and_classify.far"

grammar_name: "TokenizerClassifier"

rules { main: "TOKENIZE_AND_CLASSIFY" }
</pre>

The verbalizer configuration is similar, except that the rules can also contain
a redup entry to specify REDUP rules as discussed above (for the old serialization version pre-1.0):

<pre>
grammar_file: "en_toy/verbalize/verbalize.far"

grammar_name: "Verbalizer"

rules { main: "ALL" redup: "REDUP" }
</pre>
Note that the REDUP rule must be associated with a specific main rule.

Simple command-line interface
-------------------------

Sparrowhawk is mostly a library that is intended to be integrated into a larger
system, such as a TTS system. However, for debugging purposes we provide a
binary, "normalizer&#x005f;main", that takes the following arguments:

<pre>
normalizer_main
PROGRAM FLAGS:

  --config: type = string, default = ""
  Path to the configuration proto.
  --multi_line_text: type = bool, default = false
  Text is spread across multiple lines.
  --path_prefix: type = string, default = "./"
  Optional path prefix if not relative.
</pre>

For example in the "grammars" directory, assuming one has built all the grammars:

<pre>
normalizer_main --config=sparrowhawk_configuration.ascii_proto --multi_line_text < test.txt  2>/dev/null
</pre>

For the new serialization specification, the invocation is as follows:

<pre>
normalizer_main --config=sparrowhawk_configuration_serialization.ascii_proto --multi_line_text < test.txt 2>/dev/null
</pre>

Integrating Sparrowhawk with Festival
-------------------------

Sparrowhawk has only been tested with Festival 2.4.
To use sparrowhawk with this release you first need to install the sparrowhawk
library and its dependencies.

You may need to configure libfst as follows:
<pre>
./configure  LDFLAGS=-Wl,--no-as-needed -enable-far=true -enable-pdt=true --enable-mpdt
</pre>

Unpack speech_tools, and run .configure
Before running make, edit speech_tools/include and comment out
(at the end of the file):
<pre>
//   template<class T>
//     extern EST_TVector<T> &set(EST_TVector<T> &v, const EST_TList<T> &s);
</pre>

then run make.

Unpack the fesival tar file and
apply the sparrowhawk patch (in the "data" directory), and run configure.
Festival versions later than 2.4 should not need the patch. Sparrowhawk
integration code should be included in the release.

<pre>
  tar xvf festival-2.4-release.tar.gz
  patch -p1 < festival-2.4_sparrowhawk.patch
  cd festival
  ./configure
</pre>
Now, edit config/config and uncomment the following two lines, changing the
path if necessary:
<pre>
  INCLUDE_MODULES += Sparrowhawk
  FST_LIB = /usr/local/lib/fst
</pre>
Now make Festival:
<pre>
  make
</pre>

To run Festival you may need to set LD_LIBRARY_PATH, for example:
<pre>
  export LD_LIBRARY_PATH=/usr/local/lib
</pre>

An example basic Festival voice configuration using sparrowhawk is given in
festival/examples/sparrowhawk_test_us_null.scm

Sparrowhawk will perform tokenization and text normalization and leave you with
a sequence of words in Festival's 'Word' relation. You need to take it from
there.

How to cite Sparrowhawk
-------------------------

If you use Sparrowhawk in your project we request that you cite the GitHub page at
<https://github.com/google/sparrowhawk/> as the source. For the basic approach
used in Sparrowhawk, please cite the Ebden and Sproat (2014) paper listed
below.

References
-------------------------

Peter Ebden and Richard Sproat.  2014. "The Kestrel TTS Text Normalization System."
*Journal of Natural Language Engineering*.

Kyle Gorman and Richard Sproat. 2016. "Minimally supervised models for number
normalization." *Transactions of the Association for Computational
Linguistics*.
