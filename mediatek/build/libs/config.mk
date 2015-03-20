# config.mk - add supports for configuration file generation

MTK_AUTOCONFIG_LIST := $(patsubst $(call relative-path,../../config/common/autoconfig/)%,%,\
  $(call wildcard2,$(call relative-path,../../config/common/autoconfig/*)))

#internal functions supporting configuration generation
define .mtk.config.delete-rule
$(1): $(2).delete
$(2).delete: 
	@echo "[DELETE] $(2)"
	@rm -rf $(2)
endef

# Only show message in custgen
define .mtk.config.generate-rule
$(1): $(2)
$(2): $(3)
	@$(if $(filter 0,$(MAKELEVEL)),,echo "[CONFIG] copy $(3)")
	@$(if $(filter 0,$(MAKELEVEL)),,echo "           to $(2)")
	@mkdir -p $(dir $(2))
	@cp -f $(3) $(2)
endef

define .mtk.config.generate-auto-merge-rules
$(1): $(2)
$(2): PRIVATE_FILE_LIST := $(3)
$(2): $(3) $(4)
	@echo "[CONFIG] generate $(2)"
	@mkdir -p $(dir $(2))
	@python $(4) $(3) > $(2)
endef

define .mtk.config.generate-auto-rules
$(eval include $(MTK_ROOT_CONFIG)/common/autoconfig/$(1)/config.mk) \
$(eval _merge_order_ := $(call wildcard2,$(foreach m,$(merge-order),$(foreach p,\
  $(call reverse,$(MTK_CONFIG_FOLDERS)),$(MTK_ROOT_CONFIG)/$(p)/autoconfig/$(1)/$(m))))) \
$(eval _target_file_ := $(MTK_ROOT_GEN_CONFIG)/$(1)) \
$(if $(call seq,auto-merge,$(config-type)),$(eval \
  $(call .mtk.config.generate-auto-merge-rules,$(2),$(_target_file_),$(_merge_order_)\
         ,$(merge-tool))
),$(eval #################### add your own config file parsing rule here ###################### )
)$(_target_file_)
endef

# * mtk.config.generate-rules - create rules for auto generating configuration files
# parameter: none
# return:    auto generated file list
#define mtk.config.generate-rules
#endef


define mtk.projectconfig.generate-auto-rules
$(eval \
$(MTK_ROOT_CONFIG_OUT)/ProjectConfig.mk: $(MTK_PROJECT_CONFIGS)
ifeq ($(MTK_DEPENDENCY_AUTO_CHECK), true)
	-@echo [Update] $$@: $$?
else
	@echo "[CONFIG] generate $$@"
endif
	$$(hide) if [ -e $$@ ]; then chmod u+w $$@; else mkdir -p $$(dir $$@); fi
	$$(hide) python $$(MTK_ROOT_BUILD)/tools/config/merge-project.py $$(MTK_PROJECT_CONFIGS) > $$@
)
endef

# * mtk.config.generate-rules - generate rules for.config folder generation
# example usage
# a. associate.config files with target "all"
#    $(call mtk.config.generate-rules,all)
define mtk.config.generate-rules
$(if $(filter yes, $(strip $(BUILD_KERNEL))), \
     $(foreach c,$(MTK_AUTOCONFIG_LIST),$(call .mtk.config.generate-auto-rules,$(c),$(1))), \
) \
$(if $(MTK_ROOT_CONFIG),$(strip \
  $(eval _confflist_ :=) $(eval _conffmap_  :=)  \
  $(foreach d,$(addprefix $(MTK_ROOT_CONFIG)/,$(MTK_CONFIG_FOLDERS)),\
    $(eval _dirs := $(if $(CUSTOMER),$(filter-out $(MTK_ROOT_CONFIG)/$(CUSTOMER),$(d)),$(d))) \
   $(if $(_dirs),\
      $(eval _files := $(filter-out $(_confflist_), \
        $(patsubst $(d)/%,%,$(shell find -L $(_dirs) \( ! -name .\* \) -type f | \
           grep -v "autoconfig/\|ProjectConfig\.mk\|ProjectConfig_ckt\.mk")))) \
      $(foreach f,$(_files), \
        $(eval _confflist_ += $(f)) \
        $(eval _conffmap_  += $(MTK_ROOT_CONFIG_OUT)/$(f):$(d)/$(f)) \
      ) \
    ,) \
  ) \
  $(if $(call wildcard2,$(MTK_ROOT_CONFIG_OUT)),\
    $(foreach f,$(filter-out $(MTK_ROOT_CONFIG_OUT)/ProjectConfig.mk \ 
                             $(MTK_ROOT_CONFIG_OUT)/ProjectConfig_ckt.mk \ 
                             $(call wildcard2,$(MTK_ROOT_CONFIG_OUT)/autoconfig/*) \
                             $(foreach f,$(_conffmap_),$(word 1,$(subst :, ,$f))),\
      $(shell find $(if $(2),$(addprefix $(MTK_ROOT_CONFIG_OUT)/,$(2)),$(MTK_ROOT_CONFIG_OUT)) \
          -type f 2> /dev/null)),\
      $(eval $(call .mtk.config.delete-rule,$(1),$(f)))) \
  ,) \
  $(foreach f,$(_conffmap_),\
    $(eval $(call .mtk.config.generate-rule,$(1),$(word 1,$(subst :, ,$(f))),$(word 2,$(subst :, ,$(f))))) \
    $(word 1,$(subst :, ,$(f))) \
)),)
endef
# Example Config File
# prepare mediatek/config/common/xxx/config.mk with following content:
# config-type   := auto-merge
# merge-tool := $(MTK_ROOT_BUILD)/tools/config/merge.py
# merge-order   := \
#     common \
#     platform \
#     $(if $(seq has-feature,wifi),wifi,) \
#     BT \
#     AEE \
#     project \
#     flavor
#     ...
# 

# Example Usage
# include ../Makefile
# $(call codebase-path)
# .PHONY: mtk-config-files
# *: mtk-config-folder
# mtk-config-folder: mtk-config-files
# mtk-config-files := $(strip $(call mtk.config.generate-rules,mtk-config-files))
# config-files: ;

