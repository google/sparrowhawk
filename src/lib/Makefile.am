# Need both because of the output of protoc
AM_CPPFLAGS =  -I$(srcdir)/../include -I$(srcdir)/../include/sparrowhawk

lib_LTLIBRARIES = libsparrowhawk.la
proto_sources = items.pb.cc \
                links.pb.cc \
                rule_order.pb.cc \
                semiotic_classes.pb.cc \
                serialization_spec.pb.cc \
                sparrowhawk_configuration.pb.cc

libsparrowhawk_la_SOURCES = field_path.cc \
                            io_utils.cc \
                            normalizer.cc \
                            normalizer_utils.cc \
                            numbers.cc \
                            protobuf_parser.cc \
                            protobuf_serializer.cc \
                            record_serializer.cc \
                            regexp.cc \
                            rule_system.cc \
                            sentence_boundary.cc \
                            spec_serializer.cc \
                            string_utils.cc \
                            style_serializer.cc \
			    $(proto_sources)

libsparrowhawk_la_LDFLAGS = -version-info 0:0:0

items.pb.cc:
	$(MAKE) -C $(srcdir)/../proto/ items.pb.cc

links.pb.cc:
	$(MAKE) -C $(srcdir)/../proto/ links.pb.cc

rule_order.pb.cc:
	$(MAKE) -C $(srcdir)/../proto/ rule_order.pb.cc

semiotic_classes.pb.cc:
	$(MAKE) -C $(srcdir)/../proto/ semiotic_classes.pb.cc

serialization_spec.pb.cc:
	$(MAKE) -C $(srcdir)/../proto/ serialization_spec.pb.cc

sparrowhawk_configuration.pb.cc:
	$(MAKE) -C $(srcdir)/../proto/ sparrowhawk_configuration.pb.cc

