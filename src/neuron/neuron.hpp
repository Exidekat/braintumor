#pragma once

#include "base.hpp"

#include <cinttypes>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <variant>

#include <vk_mem_alloc.h>

namespace neuron {
    std::string NEURON_API get_version();

    enum class DeviceSelectionStrategy { Naive, FixedIndex, CustomStrategy };

    struct FixedIndexStrategy {
        size_t index;
    };

    struct CustomStrategy {
        std::function<vk::PhysicalDevice(const std::vector<vk::PhysicalDevice> &)> selector;
    };

    struct OptionalFeatureSet {};

    using ValidationCallbackFn =
        std::function<bool(vk::DebugUtilsMessageSeverityFlagBitsEXT, vk::DebugUtilsMessageTypeFlagsEXT, const vk::DebugUtilsMessengerCallbackDataEXT *, void *)>;

    struct Version {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;
    };

    struct ContextSettings {
        std::string application_name = "Application";
        Version     application_version{0, 0, 1};

        bool enable_api_validation = false;
        bool enable_api_dump       = false;

        std::optional<ValidationCallbackFn> custom_validation_callback  = std::nullopt;
        void                               *custom_validation_user_data = nullptr;

        std::unordered_set<std::string> extra_layers;
        std::unordered_set<std::string> extra_instance_extensions;
        std::unordered_set<std::string> extra_device_extensions;

        DeviceSelectionStrategy                                          device_selection_strategy = DeviceSelectionStrategy::Naive;
        std::variant<std::monostate, FixedIndexStrategy, CustomStrategy> device_selection_strategy_impl;

        OptionalFeatureSet optional_features;


        NEURON_API ContextSettings &set_naive_device_selection();
        NEURON_API ContextSettings &set_device_index_selection(size_t index);
        NEURON_API ContextSettings &set_custom_device_selector(std::function<vk::PhysicalDevice(const std::vector<vk::PhysicalDevice> &)> selector);
    };

    struct DebugUserData {
        ValidationCallbackFn fn;
        void                *user_data;
    };

    template <typename T>
    struct VmaAllocated {
        T                 resource;
        VmaAllocation     allocation;
        VmaAllocationInfo allocation_info;
    };

    class CommandPool;

    class NEURON_API Context final : public std::enable_shared_from_this<Context> {
        explicit Context(const ContextSettings &settings);
      public:

        static inline std::shared_ptr<Context> create(const ContextSettings& settings) {
            auto ctx = std::shared_ptr<Context>(new Context(settings));
            ctx->setup(ctx);
            return ctx;
        };

        ~Context();

        [[nodiscard]] vk::Instance                              instance() const;
        [[nodiscard]] std::optional<vk::DebugUtilsMessengerEXT> debug_messenger() const;
        [[nodiscard]] vk::PhysicalDevice                        physical_device() const;
        [[nodiscard]] vk::Device                                device() const;
        [[nodiscard]] vk::Queue                                 main_queue() const;
        [[nodiscard]] uint32_t                                  main_queue_family() const;
        [[nodiscard]] vk::Queue                                 transfer_queue() const;
        [[nodiscard]] uint32_t                                  transfer_queue_family() const;
        [[nodiscard]] vk::Queue                                 compute_queue() const;
        [[nodiscard]] uint32_t                                  compute_queue_family() const;
        [[nodiscard]] vk::PipelineCache                         pipeline_cache() const;
        [[nodiscard]] VmaAllocator                              allocator() const;

        [[nodiscard]] VmaAllocated<vk::Image>  allocate_image(const vk::ImageCreateInfo &ici, const VmaAllocationCreateInfo &allocation_create_info) const;
        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_buffer(const vk::BufferCreateInfo &bci, const VmaAllocationCreateInfo &allocation_create_info) const;

        void free_image(const VmaAllocated<vk::Image> &image) const;
        void free_buffer(const VmaAllocated<vk::Buffer> &buffer) const;

        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_gpu_buffer(size_t size, const void *data, vk::BufferUsageFlags usage) const;

        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_staging_buffer(size_t size, const void *data, vk::BufferUsageFlags usage) const;

        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_host_buffer(size_t size, const void *data, vk::BufferUsageFlags usage) const;

        [[nodiscard]] void *map_buffer(const VmaAllocated<vk::Buffer> &buffer) const;
        void                unmap_buffer(const VmaAllocated<vk::Buffer> &buffer) const;

        void copy_buffer_to_buffer(const VmaAllocated<vk::Buffer> &src, const VmaAllocated<vk::Buffer> &dst, vk::DeviceSize size, vk::DeviceSize src_offset, vk::DeviceSize dst_offset) const;


        template<typename T>
        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_gpu_buffer(const std::vector<T>& v, vk::BufferUsageFlags usage) const {
            return  allocate_gpu_buffer(v.size() * sizeof(T), v.data(), usage);
        };

        template<typename T>
        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_staging_buffer(const std::vector<T>& v, vk::BufferUsageFlags usage) const {
            return  allocate_staging_buffer(v.size() * sizeof(T), v.data(), usage);
        };

        template<typename T>
        [[nodiscard]] VmaAllocated<vk::Buffer> allocate_host_buffer(const std::vector<T>& v, vk::BufferUsageFlags usage) const {
            return  allocate_host_buffer(v.size() * sizeof(T), v.data(), usage);
        };

        [[nodiscard]] VmaAllocated<vk::Image> allocate_gpu_image(const void* data, const vk::Extent2D& extent, vk::Format format) const;

      private:

        void setup(const std::shared_ptr<Context>& me);

        vk::Instance                              m_instance;
        std::optional<vk::DebugUtilsMessengerEXT> m_debug_messenger;
        vk::PhysicalDevice                        m_physical_device;
        vk::Device                                m_device;

        vk::Queue m_main_queue;
        vk::Queue m_transfer_queue;
        vk::Queue m_compute_queue;

        uint32_t m_main_queue_family;
        uint32_t m_transfer_queue_family;
        uint32_t m_compute_queue_family;

        OptionalFeatureSet m_optional_features;
        DebugUserData     *m_debug_user_data = nullptr;

        vk::PipelineCache m_pipeline_cache = VK_NULL_HANDLE;

        VmaAllocator m_allocator;

        std::shared_ptr<CommandPool> m_main_pool;
        std::shared_ptr<CommandPool> m_transfer_pool;
        std::shared_ptr<CommandPool> m_compute_pool;
    };

    class NEURON_API CommandPool {
      public:
         CommandPool(const std::shared_ptr<Context> &context, uint32_t queue_family, bool resettable = false);
        ~CommandPool();

        [[nodiscard]] std::vector<vk::CommandBuffer> allocate_command_buffers(uint32_t count, vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;
        [[nodiscard]] vk::CommandBuffer              allocate_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;

        void free_command_buffers(const std::vector<vk::CommandBuffer> &command_buffers) const;

      private:
        std::shared_ptr<Context> m_context;
        vk::CommandPool          m_command_pool;
    };

} // namespace neuron
