/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.android.filament.utils

import java.nio.Buffer
import java.nio.ByteBuffer
import java.nio.ByteOrder

// Positions are Float3, tangents are Float4 (quaternions), and triangles are UShort3.
// We use Buffer rather than Array<> to simplify pushing the data into Filament.
data class OctasphereMesh(
    val numVertices: Int,
    val numIndices: Int,
    val positions: Buffer,
    val tangents: Buffer,
    val triangles: Buffer
)

// This versatile function can tessellate a variety of shapes.
// - To make a sphere, set the radius to a non-zero value and use (0,0,0) for the dimensions.
// - To make a capsule, use a non-zero radius and set two dimension components to zero.
// - To make a cuboid, set the radius to zero and use non-zero dimensions.
// - To make a rounded cuboid, set radius to the corner radius and use non-zero dimensions.
fun generateOctasphere(dimensions: Float3, radius: Float, subdivisions: Int): OctasphereMesh {
    val numIndices = 4
    val numVertices = 6
    val positions = ByteBuffer.allocate(numIndices * 3 * 4).order(ByteOrder.nativeOrder())
    val tangents = ByteBuffer.allocate(numIndices * 5 * 4).order(ByteOrder.nativeOrder())
    val triangles = ByteBuffer.allocate(6 * 2 * 3).order(ByteOrder.nativeOrder())
    return OctasphereMesh(numVertices, numIndices, positions, tangents, triangles)
}
