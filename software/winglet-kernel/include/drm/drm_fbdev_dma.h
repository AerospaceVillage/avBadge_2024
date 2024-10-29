/* SPDX-License-Identifier: MIT */

#ifndef DRM_FBDEV_DMA_H
#define DRM_FBDEV_DMA_H

struct drm_device;

#ifdef CONFIG_DRM_FBDEV_EMULATION
void drm_fbdev_dma_setup(struct drm_device *dev, unsigned int preferred_bpp);
void drm_fbdev_dma_setup2(struct drm_device *dev, unsigned int preferred_bpp,
			  unsigned int override_w, unsigned int override_h,
			  unsigned int override_flags);
#else
static inline void drm_fbdev_dma_setup(struct drm_device *dev, unsigned int preferred_bpp)
{ }
static inline void drm_fbdev_dma_setup2(struct drm_device *dev, unsigned int preferred_bpp,
			  unsigned int override_w, unsigned int override_h,
			  unsigned int override_flags) { }
#endif

#endif
