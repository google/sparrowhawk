BUILT_SOURCES = $(srcdir)/sparrowhawk/items.pb.h $(srcdir)/sparrowhawk/links.pb.h \
                $(srcdir)/sparrowhawk/rule_order.pb.h \
                $(srcdir)/sparrowhawk/semiotic_classes.pb.h \
                $(srcdir)/sparrowhawk/serialization_spec.pb.h \
                $(srcdir)/sparrowhawk/sparrowhawk_configuration.pb.h

nobase_include_HEADERS =  sparrowhawk/field_path.h \
		          sparrowhawk/io_utils.h \
		          sparrowhawk/logger.h \
		          sparrowhawk/normalizer.h \
		          sparrowhawk/numbers.h \
		          sparrowhawk/protobuf_parser.h \
		          sparrowhawk/protobuf_serializer.h \
		          sparrowhawk/record_serializer.h \
		          sparrowhawk/regexp.h \
		          sparrowhawk/rule_system.h \
		          sparrowhawk/sentence_boundary.h \
		          sparrowhawk/spec_serializer.h \
		          sparrowhawk/string_utils.h \
		          sparrowhawk/style_serializer.h \
		          $(BUILT_SOURCES)

sparrowhawk/items.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ items.pb.h

sparrowhawk/links.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ links.pb.h

sparrowhawk/rule_order.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ rule_order.pb.h

sparrowhawk/semiotic_classes.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ semiotic_classes.pb.h

sparrowhawk/serialization_spec.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ serialization_spec.pb.h

sparrowhawk/sparrowhawk_configuration.pb.h:
	$(MAKE) -C $(srcdir)/../proto/ sparrowhawk_configuration.pb.h
 
