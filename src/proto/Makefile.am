dist_noinst_DATA = items.proto \
                   links.proto \
                   rule_order.proto \
                   semiotic_classes.proto \
	           serialization_spec.proto \
                   sparrowhawk_configuration.proto

CC_OUT = $(srcdir)/../lib
H_OUT  = $(srcdir)/../include/sparrowhawk

%.pb.cc %.pb.h: %.proto
	$(PROTOC) --proto_path=$(srcdir) --cpp_out=$(srcdir) $^
	cp $*.pb.h $(H_OUT)
	cp $*.pb.cc $(CC_OUT)

MOSTLYCLEANFILES = items.pb.h items.pb.cc \
                   links.pb.h links.pb.cc \
                   rule_order.pb.h rule_order.pb.cc \
                   semiotic_classes.pb.h semiotic_classes.pb.cc \
	           serialization_spec.pb.h serialization_spec.pb.cc \
                   sparrowhawk_configuration.pb.h sparrowhawk_configuration.pb.cc

all: $(MOSTLYCLEANFILES)

