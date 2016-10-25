/* Copyright (c) 2009, 2010, 2011, 2016 Nicira, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OVSDB_ELECT_H
#define OVSDB_ELECT_H 1

typedef void ovsdb_elect_active_change_func(char *sync_from, int is_backup, void *config_);

void ovsdb_elect_init(char *local_domain_, void *sync_from, void *is_backup);
void ovsdb_elect_init_unixctl_client(const char *path);
void ovsdb_elect_fini(void);

#endif /* elect.h */

