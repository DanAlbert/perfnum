all: compute manage report

compute:
	make -f compute.mk

manage:
	make -f manage.mk

report:
	make -f report.mk

clean:
	make -f compute.mk clean
	make -f manage.mk clean
	make -f report.mk clean

.PHONY: compute manage report
