const ivec2 FaceTextureCoords[4] = ivec2[](ivec2(0, 0), ivec2(1, 0), ivec2(1, 1), ivec2(0, 1));

const ivec3 FaceOrigins[6] = ivec3[](ivec3(1, 0, 1),  // Positive X
                                     ivec3(0, 0, 0),  // Negative X
                                     ivec3(0, 1, 1),  // Positive Y
                                     ivec3(0, 0, 0),  // Negative Y
                                     ivec3(0, 0, 1),  // Positive Z
                                     ivec3(1, 0, 0)); // Negative Z

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

const vec3 FaceNormals[6] = vec3[](vec3(1.0, 0.0, 0.0),
                                   vec3(-1.0, 0.0, 0.0),
                                   vec3(0.0, 1.0, 0.0),
                                   vec3(0.0, -1.0, 0.0),
                                   vec3(0.0, 0.0, 1.0),
                                   vec3(0.0, 0.0, -1.0));
