#include "kvm/fdt.h"
#include "kvm/kvm.h"
#include "kvm/kvm-cpu.h"
#include "kvm/util.h"

#include "arm-common/gic.h"

#include "asm/pmu.h"

static void set_pmu_attr(struct kvm_cpu *vcpu, void *addr, u64 attr)
{
	struct kvm_device_attr pmu_attr = {
		.group	= KVM_ARM_VCPU_PMU_V3_CTRL,
		.addr	= (u64)addr,
		.attr	= attr,
	};
	int ret;

	ret = ioctl(vcpu->vcpu_fd, KVM_HAS_DEVICE_ATTR, &pmu_attr);
	if (!ret) {
		ret = ioctl(vcpu->vcpu_fd, KVM_SET_DEVICE_ATTR, &pmu_attr);
		if (ret)
			die_perror("PMU KVM_SET_DEVICE_ATTR");
	} else {
		die_perror("PMU KVM_HAS_DEVICE_ATTR");
	}
}

void pmu__generate_fdt_nodes(void *fdt, struct kvm *kvm)
{
	const char compatible[] = "arm,armv8-pmuv3";
	int irq = KVM_ARM_PMUv3_PPI;
	struct kvm_cpu *vcpu;
	int i;

	u32 cpu_mask = (((1 << kvm->nrcpus) - 1) << GIC_FDT_IRQ_PPI_CPU_SHIFT) \
		       & GIC_FDT_IRQ_PPI_CPU_MASK;
	u32 irq_prop[] = {
		cpu_to_fdt32(GIC_FDT_IRQ_TYPE_PPI),
		cpu_to_fdt32(irq - 16),
		cpu_to_fdt32(cpu_mask | IRQ_TYPE_LEVEL_HIGH),
	};

	if (!kvm->cfg.arch.has_pmuv3)
		return;

	for (i = 0; i < kvm->nrcpus; i++) {
		vcpu = kvm->cpus[i];
		set_pmu_attr(vcpu, &irq, KVM_ARM_VCPU_PMU_V3_IRQ);
		set_pmu_attr(vcpu, NULL, KVM_ARM_VCPU_PMU_V3_INIT);
	}

	_FDT(fdt_begin_node(fdt, "pmu"));
	_FDT(fdt_property(fdt, "compatible", compatible, sizeof(compatible)));
	_FDT(fdt_property(fdt, "interrupts", irq_prop, sizeof(irq_prop)));
	_FDT(fdt_end_node(fdt));
}