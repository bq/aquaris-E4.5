################################################################################
#
################################################################################

#-----------------------------------------------------------
# MTKCAM_HAVE_RR_PRIORITY
MTKCAM_HAVE_RR_PRIORITY      ?= 0  # built-in if '1' ; otherwise not built-in
MTKCAM_CFLAGS += -DMTKCAM_HAVE_RR_PRIORITY=$(MTKCAM_HAVE_RR_PRIORITY)


