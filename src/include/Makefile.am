BUILT_SOURCES = $(srcdir)/sparrowhawk/items.pb.h $(srcdir)/sparrowhawk/links.pb.h \
                $(srcdir)/sparrowhawk/rule_order.pb.h \
                $(srcdir)/sparrowhawk/semiotic_classes.pb.h \
                $(srcdir)/sparrowhawk/sparrowhawk_configuration.pb.h

nobase_include_HEADERS =  sparrowhawk/io_utils.h sparrowhawk/normalizer.h \
                          sparrowhawk/protobuf_parser.h sparrowhawk/regexp.h \
                          sparrowhawk/sentence_boundary.h sparrowhawk/logger.h \
                          sparrowhawk/numbers.h sparrowhawk/protobuf_serializer.h \
                          sparrowhawk/rule_system.h sparrowhawk/string_utils.h \
                          $(BUILT_SOURCES)

sparrowhawk/items.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ items.pb.h

sparrowhawk/links.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ links.pb.h

sparrowhawk/rule_order.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ rule_order.pb.h

sparrowhawk/semiotic_classes.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ semiotic_classes.pb.h

sparrowhawk/sparrowhawk_configuration.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ sparrowhawk_configuration.pb.h
 
