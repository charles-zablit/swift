//===--- InstructionUtils.h - Utilities for SIL instructions ----*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef SWIFT_SIL_WRAPPERTYPES_H
#define SWIFT_SIL_WRAPPERTYPES_H

#include "swift/SIL/SILFunction.h"
#include "swift/SIL/SILInstruction.h"

namespace swift {
/// An abstraction over LoadInst/LoadBorrowInst so one can handle both types of
/// load using common code.
struct LoadOperation {
  llvm::PointerUnion<LoadInst *, LoadBorrowInst *> value;

  LoadOperation() : value() {}
  LoadOperation(SILInstruction *input) : value(nullptr) {
    if (auto *li = dyn_cast<LoadInst>(input)) {
      value = li;
      return;
    }

    if (auto *lbi = dyn_cast<LoadBorrowInst>(input)) {
      value = lbi;
      return;
    }
  }

  explicit operator bool() const { return !value.isNull(); }

  SingleValueInstruction *getLoadInst() const {
    if (isa<LoadInst *>(value))
      return cast<LoadInst *>(value);
    return cast<LoadBorrowInst *>(value);
  }

  SingleValueInstruction *operator*() const { return getLoadInst(); }

  const SingleValueInstruction *operator->() const { return getLoadInst(); }

  SingleValueInstruction *operator->() { return getLoadInst(); }

  SILValue getOperand() const {
    if (isa<LoadInst *>(value))
      return cast<LoadInst *>(value)->getOperand();
    return cast<LoadBorrowInst *>(value)->getOperand();
  }

  /// Return the ownership qualifier of the underlying load if we have a load or
  /// None if we have a load_borrow.
  ///
  /// TODO: Rather than use an optional here, we should include an invalid
  /// representation in LoadOwnershipQualifier.
  std::optional<LoadOwnershipQualifier> getOwnershipQualifier() const {
    if (value.dyn_cast<LoadBorrowInst *>()) {
      return std::nullopt;
    }

    return cast<LoadInst *>(value)->getOwnershipQualifier();
  }
};

/// A wrapper type for writing generic code against conversion instructions.
///
/// Forwards a single operand in first operand position to a single result. 
struct ConversionOperation {
  SingleValueInstruction *inst = nullptr;

  ConversionOperation() = default;

  explicit ConversionOperation(SILInstruction *inst) {
    auto *svi = dyn_cast<SingleValueInstruction>(inst);
    if (!svi) {
      return;
    }
    if (!ConversionOperation::isa(svi)) {
      return;
    }
    this->inst = svi;
  }

  explicit ConversionOperation(SILValue value) {
    auto *inst = value->getDefiningInstruction();
    if (!inst) {
      return;
    }
    auto *svi = dyn_cast<SingleValueInstruction>(inst);
    if (!svi) {
      return;
    }
    if (!ConversionOperation::isa(svi)) {
      return;
    }
    this->inst = svi;
  }

  operator bool() const { return inst != nullptr; }

  SingleValueInstruction *operator->() { return inst; }
  SingleValueInstruction *operator->() const { return inst; }
  SingleValueInstruction *operator*() { return inst; }
  SingleValueInstruction *operator*() const { return inst; }

  static bool isa(SILInstruction *inst) {
    switch (inst->getKind()) {
    case SILInstructionKind::MarkUnresolvedNonCopyableValueInst:
    case SILInstructionKind::MarkUninitializedInst:
    case SILInstructionKind::ConvertFunctionInst:
    case SILInstructionKind::UpcastInst:
    case SILInstructionKind::AddressToPointerInst:
    case SILInstructionKind::UncheckedTrivialBitCastInst:
    case SILInstructionKind::UncheckedAddrCastInst:
    case SILInstructionKind::UncheckedBitwiseCastInst:
    case SILInstructionKind::RefToRawPointerInst:
    case SILInstructionKind::RawPointerToRefInst:
    case SILInstructionKind::ConvertEscapeToNoEscapeInst:
    case SILInstructionKind::RefToBridgeObjectInst:
    case SILInstructionKind::BridgeObjectToRefInst:
    case SILInstructionKind::BridgeObjectToWordInst:
    case SILInstructionKind::ThinToThickFunctionInst:
    case SILInstructionKind::ThickToObjCMetatypeInst:
    case SILInstructionKind::ObjCToThickMetatypeInst:
    case SILInstructionKind::ObjCMetatypeToObjectInst:
    case SILInstructionKind::ObjCExistentialMetatypeToObjectInst:
    case SILInstructionKind::UnconditionalCheckedCastInst:
    case SILInstructionKind::UncheckedRefCastInst:
    case SILInstructionKind::UncheckedValueCastInst:
    case SILInstructionKind::RefToUnmanagedInst:
    case SILInstructionKind::RefToUnownedInst:
    case SILInstructionKind::UnmanagedToRefInst:
    case SILInstructionKind::UnownedToRefInst:
    case SILInstructionKind::CopyableToMoveOnlyWrapperValueInst:
    case SILInstructionKind::MoveOnlyWrapperToCopyableValueInst:
    case SILInstructionKind::MoveOnlyWrapperToCopyableBoxInst:
    case SILInstructionKind::DropDeinitInst:
      return true;
    default:
      return false;
    }
  }

  SILValue getConverted() { return inst->getOperand(0); }
};

/// A wrapper type for writing generic code against SelectEnumAddrInst and
/// SelectEnumInst.
///
/// We use this instead of SelectEnumInstBase in order to avoid the need for
/// templating SelectEnumInstBase.
class SelectEnumOperation {
  PointerUnion<SelectEnumAddrInst *, SelectEnumInst *> value;

public:
  SelectEnumOperation(SelectEnumAddrInst *seai) : value(seai) {}
  SelectEnumOperation(SelectEnumInst *seai) : value(seai) {}
  SelectEnumOperation(SILInstruction *i) : value(nullptr) {
    if (auto *seai = dyn_cast<SelectEnumAddrInst>(i)) {
      value = seai;
      return;
    }

    if (auto *sei = dyn_cast<SelectEnumInst>(i)) {
      value = sei;
      return;
    }
  }

  SelectEnumOperation(const SILInstruction *i)
      : SelectEnumOperation(const_cast<SILInstruction *>(i)) {}

  operator SingleValueInstruction *() const {
    if (auto *seai = value.dyn_cast<SelectEnumAddrInst *>())
      return seai;
    return cast<SelectEnumInst *>(value);
  }

  SingleValueInstruction *operator*() const {
    if (auto *seai = value.dyn_cast<SelectEnumAddrInst *>())
      return seai;
    return cast<SelectEnumInst *>(value);
  }

  SingleValueInstruction *operator->() const {
    if (auto *seai = value.dyn_cast<SelectEnumAddrInst *>())
      return seai;
    return cast<SelectEnumInst *>(value);
  }

  operator bool() const { return bool(value); }

  SILValue getOperand() {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getOperand();
    return cast<SelectEnumAddrInst *>(value)->getOperand();
  }

  SILValue getEnumOperand() { return getOperand(); }

  const Operand &getEnumOperandRef() {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getEnumOperandRef();
    return cast<SelectEnumAddrInst *>(value)->getEnumOperandRef();
  }

  unsigned getNumCases() const {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getNumCases();
    return cast<SelectEnumAddrInst *>(value)->getNumCases();
  }

  std::pair<EnumElementDecl *, SILValue> getCase(unsigned i) const {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getCase(i);
    return cast<SelectEnumAddrInst *>(value)->getCase(i);
  }

  std::pair<EnumElementDecl *, Operand *> getCaseOperand(unsigned i) const {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getCaseOperand(i);
    return cast<SelectEnumAddrInst *>(value)->getCaseOperand(i);
  }

  /// Return the value that will be used as the result for the specified enum
  /// case.
  SILValue getCaseResult(EnumElementDecl *D) {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getCaseResult(D);
    return cast<SelectEnumAddrInst *>(value)->getCaseResult(D);
  }

  Operand *getCaseResultOperand(EnumElementDecl *D) {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getCaseResultOperand(D);
    return cast<SelectEnumAddrInst *>(value)->getCaseResultOperand(D);
  }

  /// If the default refers to exactly one case decl, return it.
  NullablePtr<EnumElementDecl> getUniqueCaseForDefault();

  bool hasDefault() const {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->hasDefault();
    return cast<SelectEnumAddrInst *>(value)->hasDefault();
  }

  SILValue getDefaultResult() const {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getDefaultResult();
    return cast<SelectEnumAddrInst *>(value)->getDefaultResult();
  }

  Operand *getDefaultResultOperand() const {
    if (auto *sei = value.dyn_cast<SelectEnumInst *>())
      return sei->getDefaultResultOperand();
    return cast<SelectEnumAddrInst *>(value)->getDefaultResultOperand();
  }
};

class ForwardingOperation {
  SILInstruction *forwardingInst = nullptr;

public:
  explicit ForwardingOperation(SILInstruction *inst);

  operator bool() const { return bool(forwardingInst); }
  const SILInstruction *operator->() const { return forwardingInst; }
  SILInstruction *operator->() { return forwardingInst; }
  const SILInstruction *operator*() const { return forwardingInst; }
  SILInstruction *operator*() { return forwardingInst; }

  ValueOwnershipKind getForwardingOwnershipKind();
  bool preservesOwnership();

  // ForwardingInstruction.swift mirrors this implementation.
  Operand *getSingleForwardingOperand() const {
    switch (forwardingInst->getKind()) {
    case SILInstructionKind::TupleInst:
    case SILInstructionKind::StructInst: {
      if (forwardingInst->getNumRealOperands() != 1)
        return nullptr;
      return *forwardingInst->getRealOperands().begin();
    }
    case SILInstructionKind::LinearFunctionInst:
    case SILInstructionKind::DifferentiableFunctionInst:
      return nullptr;
    case SILInstructionKind::MarkDependenceInst:
      return &forwardingInst->getOperandRef(MarkDependenceInst::Dependent);
    case SILInstructionKind::RefToBridgeObjectInst:
      return
        &forwardingInst->getOperandRef(RefToBridgeObjectInst::ConvertedOperand);
    case SILInstructionKind::TuplePackExtractInst:
      return &forwardingInst->getOperandRef(TuplePackExtractInst::TupleOperand);
    case SILInstructionKind::BorrowedFromInst:
      return &forwardingInst->getOperandRef(0);
    default:
      int numRealOperands = forwardingInst->getNumRealOperands();
      if (numRealOperands == 0) {
        // This can happen with enum instructions that have no payload.
        return nullptr;
      }
      assert(numRealOperands == 1);
      return &forwardingInst->getOperandRef(0);
    }
  }

  ArrayRef<Operand> getForwardedOperands() const {
    // Some instructions have multiple real operands but only forward one.
    if (auto *singleForwardingOp = getSingleForwardingOperand()) {
      return *singleForwardingOp;
    }
    // All others forward all operands (for enum, this may be zero operands).
    return forwardingInst->getAllOperands();
  }

  MutableArrayRef<Operand> getForwardedOperands() {
    if (auto *singleForwardingOp = getSingleForwardingOperand()) {
      return *singleForwardingOp;
    }
    return forwardingInst->getAllOperands();
  }

  bool canForwardOwnedCompatibleValuesOnly() {
    switch (forwardingInst->getKind()) {
    case SILInstructionKind::MarkUninitializedInst:
      return true;
    default:
      return false;
    }
  }

  bool canForwardGuaranteedCompatibleValuesOnly() {
    switch (forwardingInst->getKind()) {
    case SILInstructionKind::TupleExtractInst:
    case SILInstructionKind::TuplePackExtractInst:
    case SILInstructionKind::StructExtractInst:
    case SILInstructionKind::DifferentiableFunctionExtractInst:
    case SILInstructionKind::LinearFunctionExtractInst:
      return true;
    default:
      return false;
    }
  }

  /// Return true if the forwarded value has the same representation. If true,
  /// then the result can be mapped to the same storage without a move or copy.
  bool hasSameRepresentation() const;

  /// Return true if the forwarded value is address-only either before or after
  /// forwarding.
  bool isAddressOnly() const;

  // Call \p visitor on all forwarded results of the current forwarding
  // operation.
  bool visitForwardedValues(function_ref<bool(SILValue)> visitor);
};

enum class FixedStorageSemanticsCallKind { None, CheckIndex, GetCount };

struct FixedStorageSemanticsCall {
  ApplyInst *apply = nullptr;
  FixedStorageSemanticsCallKind kind = FixedStorageSemanticsCallKind::None;

  FixedStorageSemanticsCall(SILInstruction *input) {
    auto *applyInst = dyn_cast<ApplyInst>(input);
    if (!applyInst) {
      return;
    }
    auto *callee = applyInst->getReferencedFunctionOrNull();
    if (!callee) {
      return;
    }
    for (auto &attr : callee->getSemanticsAttrs()) {
      if (attr == "fixed_storage.check_index") {
        apply = applyInst;
        kind = FixedStorageSemanticsCallKind::CheckIndex;
        break;
      } else if (attr == "fixed_storage.get_count") {
        apply = applyInst;
        kind = FixedStorageSemanticsCallKind::GetCount;
        break;
      }
    }
  }

  FixedStorageSemanticsCallKind getKind() const { return kind; }
  explicit operator bool() const { return apply != nullptr; }
  const ApplyInst *operator->() const { return apply; }
  ApplyInst *operator->() { return apply; }
};

bool isFixedStorageSemanticsCallKind(SILFunction *function);

} // end namespace swift

#endif
