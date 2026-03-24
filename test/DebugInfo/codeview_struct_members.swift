// RUN: %target-swift-frontend %s -emit-ir -g -debug-info-format=codeview -o - | %FileCheck %s
//
// Verify that struct members are included in CodeView debug info.
// Visual Studio and other PDB consumers cannot reconstruct Swift type layouts
// from module metadata (unlike LLDB), so member information must be present
// in the PDB's LF_FIELDLIST records.
//
// This is a regression test for https://github.com/swiftlang/swift/issues/87914

public struct Point {
  public var x: Int
  public var y: Int
}

public func use(_ p: Point) -> Int { return p.x + p.y }

// The struct type should be emitted with member fields x and y.
// CHECK-DAG: !DICompositeType(tag: DW_TAG_structure_type, name: "Point",
// CHECK-DAG: !DIDerivedType(tag: DW_TAG_member, name: "x",
// CHECK-DAG: !DIDerivedType(tag: DW_TAG_member, name: "y",
