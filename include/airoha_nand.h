/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * On-flash ECC control for the Airoha EN7581 parallel NAND controller.
 */

#ifndef __AIROHA_NAND_H
#define __AIROHA_NAND_H

#include <linux/errno.h>
#include <linux/types.h>

#if CONFIG_IS_ENABLED(NAND_AIROHA_EN7581)
bool airoha_nand_bridge_active(void);
int airoha_nand_get_ecc_strength(void);
int airoha_nand_set_ecc_strength(int strength);
#else
static inline bool airoha_nand_bridge_active(void)
{
	return false;
}

static inline int airoha_nand_get_ecc_strength(void)
{
	return -ENODEV;
}

static inline int airoha_nand_set_ecc_strength(int strength)
{
	return -ENODEV;
}
#endif

#endif /* __AIROHA_NAND_H */
