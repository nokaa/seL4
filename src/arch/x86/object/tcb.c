/*
 * Copyright 2014, General Dynamics C4 Systems
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(GD_GPL)
 */

#include <types.h>
#include <api/failures.h>
#include <machine/registerset.h>
#include <object/structures.h>
#include <arch/machine.h>
#include <arch/object/tcb.h>

/* NOTE: offset is either 1 or 3 */
static inline unsigned int
setMRs_lookup_failure(tcb_t *receiver, word_t* receiveIPCBuffer, lookup_fault_t luf, unsigned int offset)
{
    word_t lufType = lookup_fault_get_lufType(luf);

    assert(n_msgRegisters == 2);

    if (offset < n_msgRegisters) {
        setRegister(receiver, msgRegisters[offset], lufType + 1);
    }

    if (!receiveIPCBuffer) {
        return n_msgRegisters;
    }

    if (offset >= n_msgRegisters) {
        receiveIPCBuffer[offset + 1] = lufType + 1;
    }

    switch (lufType) {
    case lookup_fault_invalid_root:
        return offset + 1;

    case lookup_fault_missing_capability:
        receiveIPCBuffer[offset + 2] =
            lookup_fault_missing_capability_get_bitsLeft(luf);
        return offset + 2;

    case lookup_fault_depth_mismatch:
        receiveIPCBuffer[offset + 2] =
            lookup_fault_depth_mismatch_get_bitsLeft(luf);
        receiveIPCBuffer[offset + 3] =
            lookup_fault_depth_mismatch_get_bitsFound(luf);
        return offset + 3;

    case lookup_fault_guard_mismatch:
        receiveIPCBuffer[offset + 2] =
            lookup_fault_guard_mismatch_get_bitsLeft(luf);
        receiveIPCBuffer[offset + 3] =
            lookup_fault_guard_mismatch_get_guardFound(luf);
        receiveIPCBuffer[offset + 4] =
            lookup_fault_guard_mismatch_get_bitsFound(luf);
        return offset + 4;

    default:
        fail("Invalid lookup failure");
    }
}

word_t setMRs_fault(tcb_t *sender, tcb_t* receiver, word_t *receiveIPCBuffer)
{
    assert(n_msgRegisters == 2);

    switch (fault_get_faultType(sender->tcbFault)) {
    case fault_cap_fault:
        setRegister(receiver, msgRegisters[0], getRestartPC(sender));
        setRegister(receiver, msgRegisters[1],
                    fault_cap_fault_get_address(sender->tcbFault));
        if (!receiveIPCBuffer) {
            return n_msgRegisters;
        }
        receiveIPCBuffer[2 + 1] =
            fault_cap_fault_get_inReceivePhase(sender->tcbFault);
        return setMRs_lookup_failure(receiver, receiveIPCBuffer, sender->tcbLookupFailure, 3);

    case fault_vm_fault:
        setRegister(receiver, msgRegisters[0], getRestartPC(sender));
        setRegister(receiver, msgRegisters[1],
                    fault_vm_fault_get_address(sender->tcbFault));
        if (!receiveIPCBuffer) {
            return n_msgRegisters;
        }
        receiveIPCBuffer[2 + 1] =
            fault_vm_fault_get_instructionFault(sender->tcbFault);
        receiveIPCBuffer[3 + 1] = fault_vm_fault_get_FSR(sender->tcbFault);
        return 4;

    case fault_unknown_syscall: {
        word_t i;

        for (i = 0; i < n_msgRegisters; i++) {
            setRegister(receiver, msgRegisters[i],
                        getRegister(sender, syscallMessage[i]));
        }
        if (receiveIPCBuffer) {
            for (; i < n_syscallMessage; i++) {
                receiveIPCBuffer[i + 1] =
                    getRegister(sender, syscallMessage[i]);
            }

            receiveIPCBuffer[i + 1] =
                fault_unknown_syscall_get_syscallNumber(sender->tcbFault);
            return n_syscallMessage + 1;
        } else {
            return n_msgRegisters;
        }
    }

    case fault_user_exception: {
        word_t i;

        for (i = 0; i < n_msgRegisters; i++) {
            setRegister(receiver, msgRegisters[i],
                        getRegister(sender, exceptionMessage[i]));
        }
        if (receiveIPCBuffer) {
            for (; i < n_exceptionMessage; i++) {
                receiveIPCBuffer[i + 1] =
                    getRegister(sender, exceptionMessage[i]);
            }
            receiveIPCBuffer[n_exceptionMessage + 1] =
                fault_user_exception_get_number(sender->tcbFault);
            receiveIPCBuffer[n_exceptionMessage + 2] =
                fault_user_exception_get_code(sender->tcbFault);
            return n_exceptionMessage + 2;
        } else {
            return n_msgRegisters;
        }
    }

    default:
        fail("Invalid fault");
    }
}

word_t CONST Arch_decodeTransfer(word_t flags)
{
    return 0;
}

exception_t CONST Arch_performTransfer(word_t arch, tcb_t *tcb_src, tcb_t *tcb_dest)
{
    return EXCEPTION_NONE;
}
