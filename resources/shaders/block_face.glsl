// To align textures properly, all vertical faces must have their tangent vectors pointing right and
// their bitangent vectors pointing up.
const mat3 FaceTBNMatrices[6]
    = mat3[](mat3(0.0, 0.0, -1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0), // Positive X
             mat3(0.0, 0.0, 1.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0), // Negative X
             mat3(1.0, 0.0, 0.0, 0.0, 0.0, -1.0, 0.0, 1.0, 0.0), // Positive Y
             mat3(1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, -1.0, 0.0), // Negative Y
             mat3(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0),  // Positive Z
             mat3(-1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, -1.0) // Negative Z
    );

const ivec2 FaceTextureCoords[4] = ivec2[](ivec2(0, 0), ivec2(1, 0), ivec2(1, 1), ivec2(0, 1));

const ivec3 FaceTangents[6] = ivec3[](ivec3(0, 0, -1),
                                      ivec3(0, 0, 1),
                                      ivec3(1, 0, 0),
                                      ivec3(1, 0, 0),
                                      ivec3(1, 0, 0),
                                      ivec3(-1, 0, 0));

const ivec3 FaceBitangents[6] = ivec3[](ivec3(0, 1, 0),
                                        ivec3(0, 1, 0),
                                        ivec3(0, 0, -1),
                                        ivec3(0, 0, 1),
                                        ivec3(0, 1, 0),
                                        ivec3(0, 1, 0));
