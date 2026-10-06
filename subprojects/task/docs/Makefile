.PHONY: default doc doc-html doc-pdf

default: doc

doc: doc-html

doc-html: P3552-task.html P3796-task-issues.html P3941-affinity.html P3980-allocation.html

doc-pdf: P3552-task.pdf P3796-task-issues.pdf

wg21/Makefile:
	git clone https://github.com/mpark/wg21.git

distclean: clean
	$(RM) -r generated
	$(RM) mkerr olderr

include wg21/Makefile
