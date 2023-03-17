#include "vk_textures.h"
#include <iostream>

#include "vk_initializers.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


bool vkutil::load_image_from_file(VulkanEngine& engine, const char* file, Texture& outTexture, bool generateMips)
{
	stbi_uc* pixels = stbi_load(file, &outTexture._width, &outTexture._height, &outTexture._channelCount, STBI_rgb_alpha);

	if (!pixels) {
		std::cout << "Failed to load texture file " << file << std::endl;
		return false;
	}

	VkDeviceSize imageSize = outTexture._width * outTexture._height * 4;
	VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;
	AllocatedBuffer stagingBuffer = engine.create_buffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

	void* data;
	vmaMapMemory(engine._allocator, stagingBuffer._allocation, &data);
	memcpy(data, (void*)pixels, static_cast<size_t>(imageSize));
	vmaUnmapMemory(engine._allocator, stagingBuffer._allocation);
	stbi_image_free(pixels);

	VkExtent3D imageExtent;
	imageExtent.width = static_cast<uint32_t>(outTexture._width);
	imageExtent.height = static_cast<uint32_t>(outTexture._height);
	imageExtent.depth = 1;

	if (generateMips) {
		outTexture._mipLevels = floor(log2(std::max(outTexture._width, outTexture._height))) + 1;
	}
	else {
		outTexture._mipLevels = 1;
	}

	VkImageCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = image_format;
	createInfo.extent = imageExtent;
	createInfo.mipLevels = outTexture._mipLevels;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	AllocatedImage newImage;

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_AUTO;

	vmaCreateImage(engine._allocator, &createInfo, &dimg_allocinfo, &newImage._image, &newImage._allocation, nullptr);

	//transition image to transfer-receiver	
	engine.immediate_submit([&](VkCommandBuffer cmd) 
	{
		VkImageSubresourceRange range;
		range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		range.baseMipLevel = 0;
		range.levelCount = 1;
		range.baseArrayLayer = 0;
		range.layerCount = 1;
		VkImageMemoryBarrier imageBarrier_toTransfer = {};
		imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toTransfer.image = newImage._image;
		imageBarrier_toTransfer.subresourceRange = range;
		imageBarrier_toTransfer.srcAccessMask = 0;
		imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;
		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		copyRegion.imageExtent = imageExtent;

		// First 1:1 copy for mip level 1
		vkCmdCopyBufferToImage(cmd, stagingBuffer._buffer, newImage._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		// Prepare that image to be read from
		VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
		imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

		// Now walk the mip chain and copy down mips from n-1 to n
		for (int32_t i = 1; i < outTexture._mipLevels; i++)
		{
			VkImageBlit imageBlit{};

			// Source
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(outTexture._width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(outTexture._height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			// Destination
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(outTexture._width >> i);
			imageBlit.dstOffsets[1].y = int32_t(outTexture._height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			// Prepare current mip level as image blit destination
			VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;
			imageBarrier_toReadable.image = newImage._image;
			imageBarrier_toReadable.subresourceRange = mipSubRange;
			imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier_toReadable.srcAccessMask = 0;
			imageBarrier_toReadable.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);

			// Blit from previous level
			vkCmdBlitImage(cmd,	newImage._image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, newImage._image,	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

			// Prepare current mip level as image blit source for next level
			VkImageMemoryBarrier barrier2 = imageBarrier_toTransfer;
			barrier2.image = newImage._image;
			barrier2.subresourceRange = mipSubRange;
			barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier2.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier2.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier2);
		}

		// After the loop, all mip layers are in TRANSFER_SRC layout, so transition all to SHADER_READ
		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = outTexture._mipLevels;
		subresourceRange.layerCount = 1;

		// Prepare current mip level as image blit source for next level
		VkImageMemoryBarrier barrier = imageBarrier_toTransfer;
		barrier.image = newImage._image;
		barrier.subresourceRange = subresourceRange;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	});

	vmaDestroyBuffer(engine._allocator, stagingBuffer._buffer, stagingBuffer._allocation);

	outTexture.image = newImage;
	
	// Image view
	VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, outTexture.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
	imageinfo.subresourceRange.levelCount = outTexture._mipLevels;

	vkCreateImageView(engine._device, &imageinfo, nullptr, &outTexture.imageView);

	// isolate name
	std::string filepath = file;
	std::string filename = filepath.substr(filepath.rfind("/") + 1);
	outTexture._filename = filename.substr(0, filename.length() - 4);

	//std::cout << filename << " has " << outTexture._mipLevels << "mips\n";

	return true;
}
