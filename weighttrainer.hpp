// Copyright 2017 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.


// Naive SimHashing does not yield quite the performance we want. If you read
// the code for the FunctionSimHasher, you will see that the calculation of the
// per-function SimHash admits a "weight" to be applied to each observed feature.
//
// This weight can be derived / trained / learnt from existing data. A good way
// of doing this (in a slightly different setting) was described in the paper
// "Semi-Supervised SimHash for Efficient Document Similarity Search"; while the
// notation is somewhat different and focused on searching in R^m (vs over a set
// of features), our code here does something similar.
//
// We wish to optimize weights so that the following objective function is max-
// imized:
//
//


