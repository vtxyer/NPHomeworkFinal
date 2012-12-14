
/*
 * public xen defines and struct for x86_64
 * generated by mkheader.py -- DO NOT EDIT
 */

#ifndef __FOREIGN_X86_64_H__
#define __FOREIGN_X86_64_H__ 1


#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
# define __DECL_REG(name) union { uint64_t r ## name, e ## name; }
# define __align8__ __attribute__((aligned (8)))
#else
# define __DECL_REG(name) uint64_t r ## name
# define __align8__ FIXME
#endif
#define __x86_64___X86_64 1

#define FLAT_RING3_CS64_X86_64 0xe033  /* GDT index 261 */
#define FLAT_RING3_DS64_X86_64 0x0000  /* NULL selector */
#define FLAT_RING3_SS64_X86_64 0xe02b  /* GDT index 262 */
#define FLAT_KERNEL_DS64_X86_64 FLAT_RING3_DS64_X86_64
#define FLAT_KERNEL_DS_X86_64   FLAT_KERNEL_DS64_X86_64
#define FLAT_KERNEL_CS64_X86_64 FLAT_RING3_CS64_X86_64
#define FLAT_KERNEL_CS_X86_64   FLAT_KERNEL_CS64_X86_64
#define FLAT_KERNEL_SS64_X86_64 FLAT_RING3_SS64_X86_64
#define FLAT_KERNEL_SS_X86_64   FLAT_KERNEL_SS64_X86_64
#define xen_pfn_to_cr3_x86_64(pfn) ((__align8__ uint64_t)(pfn) << 12)
#define xen_cr3_to_pfn_x86_64(cr3) ((__align8__ uint64_t)(cr3) >> 12)
#define XEN_LEGACY_MAX_VCPUS_X86_64 32
#define _VGCF_i387_valid_X86_64               0
#define VGCF_i387_valid_X86_64                (1<<_VGCF_i387_valid_X86_64)
#define _VGCF_in_kernel_X86_64                2
#define VGCF_in_kernel_X86_64                 (1<<_VGCF_in_kernel_X86_64)
#define _VGCF_failsafe_disables_events_X86_64 3
#define VGCF_failsafe_disables_events_X86_64  (1<<_VGCF_failsafe_disables_events_X86_64)
#define _VGCF_syscall_disables_events_X86_64  4
#define VGCF_syscall_disables_events_X86_64   (1<<_VGCF_syscall_disables_events_X86_64)
#define _VGCF_online_X86_64                   5
#define VGCF_online_X86_64                    (1<<_VGCF_online_X86_64)
#define MAX_GUEST_CMDLINE_X86_64 1024

#define x86_64_has_no_vcpu_cr_regs 1

#define x86_64_has_no_vcpu_ar_regs 1

struct start_info_x86_64 {
    char magic[32];             
    __align8__ uint64_t nr_pages;     
    __align8__ uint64_t shared_info;  
    uint32_t flags;             
    __align8__ uint64_t store_mfn;        
    uint32_t store_evtchn;      
    union {
        struct {
            __align8__ uint64_t mfn;      
            uint32_t  evtchn;   
        } domU;
        struct {
            uint32_t info_off;  
            uint32_t info_size; 
        } dom0;
    } console;
    __align8__ uint64_t pt_base;      
    __align8__ uint64_t nr_pt_frames; 
    __align8__ uint64_t mfn_list;     
    __align8__ uint64_t mod_start;    
    __align8__ uint64_t mod_len;      
    int8_t cmd_line[MAX_GUEST_CMDLINE_X86_64];
    __align8__ uint64_t first_p2m_pfn;
    __align8__ uint64_t nr_p2m_frames;
};
typedef struct start_info_x86_64 start_info_x86_64_t;

struct trap_info_x86_64 {
    uint8_t       vector;  
    uint8_t       flags;   
    uint16_t      cs;      
    __align8__ uint64_t address; 
};
typedef struct trap_info_x86_64 trap_info_x86_64_t;

#define x86_64_has_no_pt_fpreg 1

struct cpu_user_regs_x86_64 {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    __DECL_REG(bp);
    __DECL_REG(bx);
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    __DECL_REG(ax);
    __DECL_REG(cx);
    __DECL_REG(dx);
    __DECL_REG(si);
    __DECL_REG(di);
    uint32_t error_code;    
    uint32_t entry_vector;  
    __DECL_REG(ip);
    uint16_t cs, _pad0[1];
    uint8_t  saved_upcall_mask;
    uint8_t  _pad1[3];
    __DECL_REG(flags);      
    __DECL_REG(sp);
    uint16_t ss, _pad2[3];
    uint16_t es, _pad3[3];
    uint16_t ds, _pad4[3];
    uint16_t fs, _pad5[3]; 
    uint16_t gs, _pad6[3]; 
};
typedef struct cpu_user_regs_x86_64 cpu_user_regs_x86_64_t;

#define x86_64_has_no_xen_ia64_boot_param 1

#define x86_64_has_no_ia64_tr_entry 1

#define x86_64_has_no_vcpu_tr_regs 1

#define x86_64_has_no_vcpu_guest_context_regs 1

struct vcpu_guest_context_x86_64 {
    struct { char x[512]; } fpu_ctxt;       
    __align8__ uint64_t flags;                    
    struct cpu_user_regs_x86_64 user_regs;         
    struct trap_info_x86_64 trap_ctxt[256];        
    __align8__ uint64_t ldt_base, ldt_ents;       
    __align8__ uint64_t gdt_frames[16], gdt_ents; 
    __align8__ uint64_t kernel_ss, kernel_sp;     
    __align8__ uint64_t ctrlreg[8];               
    __align8__ uint64_t debugreg[8];              
#ifdef __i386___X86_64
    __align8__ uint64_t event_callback_cs;        
    __align8__ uint64_t event_callback_eip;
    __align8__ uint64_t failsafe_callback_cs;     
    __align8__ uint64_t failsafe_callback_eip;
#else
    __align8__ uint64_t event_callback_eip;
    __align8__ uint64_t failsafe_callback_eip;
#ifdef __XEN__
    union {
        __align8__ uint64_t syscall_callback_eip;
        struct {
            unsigned int event_callback_cs;    
            unsigned int failsafe_callback_cs; 
        };
    };
#else
    __align8__ uint64_t syscall_callback_eip;
#endif
#endif
    __align8__ uint64_t vm_assist;                
#ifdef __x86_64___X86_64
    uint64_t      fs_base;
    uint64_t      gs_base_kernel;
    uint64_t      gs_base_user;
#endif
};
typedef struct vcpu_guest_context_x86_64 vcpu_guest_context_x86_64_t;

struct arch_vcpu_info_x86_64 {
    __align8__ uint64_t cr2;
    __align8__ uint64_t pad; 
};
typedef struct arch_vcpu_info_x86_64 arch_vcpu_info_x86_64_t;

struct vcpu_time_info_x86_64 {
    uint32_t version;
    uint32_t pad0;
    uint64_t tsc_timestamp;   
    uint64_t system_time;     
    uint32_t tsc_to_system_mul;
    int8_t   tsc_shift;
    int8_t   pad1[3];
};
typedef struct vcpu_time_info_x86_64 vcpu_time_info_x86_64_t;

struct vcpu_info_x86_64 {
    uint8_t evtchn_upcall_pending;
    uint8_t evtchn_upcall_mask;
    __align8__ uint64_t evtchn_pending_sel;
    struct arch_vcpu_info_x86_64 arch;
    struct vcpu_time_info_x86_64 time;
};
typedef struct vcpu_info_x86_64 vcpu_info_x86_64_t;

struct arch_shared_info_x86_64 {
    __align8__ uint64_t max_pfn;                  
    __align8__ uint64_t     pfn_to_mfn_frame_list_list;
    __align8__ uint64_t nmi_reason;
    uint64_t pad[32];
};
typedef struct arch_shared_info_x86_64 arch_shared_info_x86_64_t;

struct shared_info_x86_64 {
    struct vcpu_info_x86_64 vcpu_info[XEN_LEGACY_MAX_VCPUS_X86_64];
    __align8__ uint64_t evtchn_pending[sizeof(__align8__ uint64_t) * 8];
    __align8__ uint64_t evtchn_mask[sizeof(__align8__ uint64_t) * 8];
    uint32_t wc_version;      
    uint32_t wc_sec;          
    uint32_t wc_nsec;         
    struct arch_shared_info_x86_64 arch;
};
typedef struct shared_info_x86_64 shared_info_x86_64_t;

#endif /* __FOREIGN_X86_64_H__ */
