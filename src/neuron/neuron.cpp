#define VMA_IMPLEMENTATION

#include "neuron.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

namespace neuron {
    std::string get_version() {
        return NEURON_VERSION_STRING;
    }

    ContextSettings &ContextSettings::set_naive_device_selection() {
        device_selection_strategy      = DeviceSelectionStrategy::Naive;
        device_selection_strategy_impl = std::monostate{};
        return *this;
    }

    ContextSettings &ContextSettings::set_device_index_selection(size_t index) {
        device_selection_strategy      = DeviceSelectionStrategy::FixedIndex;
        device_selection_strategy_impl = FixedIndexStrategy{index};
        return *this;
    }

    ContextSettings &ContextSettings::set_custom_device_selector(std::function<vk::PhysicalDevice(const std::vector<vk::PhysicalDevice> &)> selector) {
        device_selection_strategy      = DeviceSelectionStrategy::CustomStrategy;
        device_selection_strategy_impl = CustomStrategy{std::move(selector)};
        return *this;
    }

    Context::Context(const ContextSettings &settings) : m_optional_features(settings.optional_features) {
        glfwInit();
        VULKAN_HPP_DEFAULT_DISPATCHER.init();

        vk::ApplicationInfo app_info{};
        app_info.setApiVersion(vk::ApiVersion13);
        app_info.setEngineVersion(vk::makeApiVersion(0, NEURON_VERSION_MAJOR, NEURON_VERSION_MINOR, NEURON_VERSION_PATCH));
        app_info.setApplicationVersion(vk::makeApiVersion(0U, settings.application_version.major, settings.application_version.minor, settings.application_version.patch));
        app_info.setPEngineName("Neuron");
        app_info.setPApplicationName(settings.application_name.c_str());

        std::unordered_set<std::string> instance_extensions_set(settings.extra_instance_extensions.begin(), settings.extra_instance_extensions.end());

        uint32_t     count;
        const char **required_extensions = glfwGetRequiredInstanceExtensions(&count);

        for (uint32_t i = 0; i < count; ++i) {
            instance_extensions_set.insert(required_extensions[i]);
        }

        // Add VK_KHR_portability_enumeration extension to the set of instance extensions
        instance_extensions_set.insert(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        std::unordered_set<std::string> layers_set(settings.extra_layers.begin(), settings.extra_layers.end());

        if (settings.enable_api_validation) {
            layers_set.insert("VK_LAYER_KHRONOS_validation");
            instance_extensions_set.insert(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        if (settings.enable_api_dump) {
            layers_set.insert("VK_LAYER_LUNARG_api_dump");
        }

        std::vector<const char *> layers;
        std::vector<const char *> instance_extensions;

        for (const auto &extension : instance_extensions_set) {
            instance_extensions.push_back(extension.c_str());
        }

        for (const auto &layer : layers_set) {
            layers.push_back(layer.c_str());
        }

        vk::InstanceCreateInfo instance_create_info{};
        instance_create_info.setPApplicationInfo(&app_info);
        instance_create_info.setPEnabledExtensionNames(instance_extensions);
        instance_create_info.setPEnabledLayerNames(layers);

        // Set VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR flag
        instance_create_info.flags = vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

        vk::DebugUtilsMessengerCreateInfoEXT messenger_create_info{};

        if (settings.enable_api_validation) {
            if (settings.custom_validation_callback.has_value()) {
                m_debug_user_data                     = new DebugUserData{.fn = settings.custom_validation_callback.value(), .user_data = settings.custom_validation_user_data};
                messenger_create_info.pfnUserCallback = +[](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                                            const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *userData) {
                    const auto *user_data = static_cast<DebugUserData *>(userData);
                    return static_cast<VkBool32>(user_data->fn(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(severity),
                                                               static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(type),
                                                               reinterpret_cast<const vk::DebugUtilsMessengerCallbackDataEXT *>(callback_data), user_data->user_data));
                };
            } else {
                messenger_create_info.pfnUserCallback = +[](VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                                            const VkDebugUtilsMessengerCallbackDataEXT *callback_data, void *userData) {
                    switch (severity) {
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
                        std::cerr << "[validation] (debug) " << callback_data->pMessage << '\n';
                        break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
                        std::cerr << "[validation] (info ) " << callback_data->pMessage << '\n';
                        break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
                        std::cerr << "[validation] (warn ) " << callback_data->pMessage << '\n';
                        break;
                    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
                        std::cerr << "[validation] (error) " << callback_data->pMessage << '\n';
                        break;
                    default:
                        // weirdly needed to escape this to avoid anyone from making a trigraph here (because those are relevant in today's world.
                        // we don't have curly braces we have curly braces at home; curly braces at home: ??<
                        std::cerr << "[validation] (????\?)" << callback_data->pMessage << '\n';
                        break;
                    }

                    return VK_FALSE;
                };
            }

            messenger_create_info.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
            messenger_create_info.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::eDeviceAddressBinding;

            instance_create_info.pNext = &messenger_create_info;
        }

        m_instance = vk::createInstance(instance_create_info);

        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);

        if (settings.enable_api_validation) {
            m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(messenger_create_info);
        }

        std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();

        switch (settings.device_selection_strategy) {
        case DeviceSelectionStrategy::Naive:
            for (const auto &physical_device : physical_devices) {
                vk::PhysicalDeviceProperties properties = physical_device.getProperties();
                if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
                    m_physical_device = physical_device;
                    ;
                    break;
                }
            }
            break;
        case DeviceSelectionStrategy::FixedIndex: {
            size_t index = std::get<FixedIndexStrategy>(settings.device_selection_strategy_impl).index;
            if (index >= physical_devices.size()) {
                throw std::runtime_error("Fixed physical device index out of range (GPU does not exist).");
            }

            m_physical_device = physical_devices[index];
        } break;
        case DeviceSelectionStrategy::CustomStrategy:
            m_physical_device = std::get<CustomStrategy>(settings.device_selection_strategy_impl).selector(physical_devices);
            break;
        }

        vk::PhysicalDeviceProperties properties = m_physical_device.getProperties();

        std::cout << "Selected GPU: " << properties.deviceName << '\n';

        // TODO: non-naive queue family selection

        std::optional<uint32_t> mqf;
        std::optional<uint32_t> tqf;
        std::optional<uint32_t> cqf;

        auto     queue_family_properties = m_physical_device.getQueueFamilyProperties();
        uint32_t qi                      = 0;
        for (const auto &qfp : queue_family_properties) {
            if (!mqf.has_value() && qfp.queueFlags & vk::QueueFlagBits::eGraphics && glfwGetPhysicalDevicePresentationSupport(m_instance, m_physical_device, qi)) {
                mqf = qi;
            }

            if (!tqf.has_value() && qfp.queueFlags & vk::QueueFlagBits::eTransfer && !(qfp.queueFlags & vk::QueueFlagBits::eGraphics) &&
                !(qfp.queueFlags & vk::QueueFlagBits::eCompute)) {
                tqf = qi;
            }

            if (!cqf.has_value() && qfp.queueFlags & vk::QueueFlagBits::eCompute && !(qfp.queueFlags & vk::QueueFlagBits::eGraphics)) {
                cqf = qi;
            }

            qi++;
        }

        m_main_queue_family     = mqf.value();
        m_transfer_queue_family = tqf.value_or(m_main_queue_family);
        m_compute_queue_family  = cqf.value_or(m_main_queue_family);

        std::array<float, 1> queue_priorities = {1.0f};

        std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
        queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, m_main_queue_family, queue_priorities);

        if (m_transfer_queue_family != m_main_queue_family) {
            queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, m_transfer_queue_family, queue_priorities);
        }

        if (m_compute_queue_family != m_main_queue_family) {
            queue_create_infos.emplace_back(vk::DeviceQueueCreateFlags{}, m_compute_queue_family, queue_priorities);
        }

        std::unordered_set<std::string> device_extensions_set(settings.extra_device_extensions.begin(), settings.extra_device_extensions.end());
        device_extensions_set.insert(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        std::vector<const char *> device_extensions;
        for (const auto &extension : device_extensions_set) {
            device_extensions.push_back(extension.c_str());
        }

        vk::PhysicalDeviceFeatures2        f2{};
        vk::PhysicalDeviceVulkan11Features v11f{};
        vk::PhysicalDeviceVulkan12Features v12f{};
        vk::PhysicalDeviceVulkan13Features v13f{};

        f2.pNext   = &v11f;
        v11f.pNext = &v12f;
        v12f.pNext = &v13f;

        v13f.dynamicRendering          = true;
        v13f.synchronization2          = true;
        v12f.timelineSemaphore         = true;
        f2.features.geometryShader     = true;
        f2.features.tessellationShader = true;
        f2.features.largePoints        = true;
        f2.features.wideLines          = true;
        f2.features.imageCubeArray     = true;

        vk::DeviceCreateInfo device_create_info{};
        device_create_info.setQueueCreateInfos(queue_create_infos);
        device_create_info.setPEnabledExtensionNames(device_extensions);
        device_create_info.setPNext(&f2);

        m_device = m_physical_device.createDevice(device_create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);


        m_main_queue     = m_device.getQueue(m_main_queue_family, 0);
        m_transfer_queue = m_device.getQueue(m_transfer_queue_family, 0);
        m_compute_queue  = m_device.getQueue(m_compute_queue_family, 0);


        // setup pipeline cache

        // TODO: use actual cache dir (do this when updating to use proper application folders)
        void  *pc_init_data      = nullptr;
        size_t pc_init_data_size = 0;
        if (std::filesystem::exists("pipeline_cache")) {
            std::ifstream f("pipeline_cache", std::ios::ate | std::ios::binary);
            pc_init_data_size = f.tellg();
            pc_init_data      = malloc(pc_init_data_size);
            f.seekg(0);
            f.read(static_cast<char *>(pc_init_data), pc_init_data_size);
            f.close();
        }

        m_pipeline_cache = m_device.createPipelineCache({{}, pc_init_data_size, pc_init_data});

        if (pc_init_data) {
            free(pc_init_data);
        }

        VmaAllocatorCreateInfo aci{};
        aci.device         = m_device;
        aci.instance       = m_instance;
        aci.physicalDevice = m_physical_device;

        vmaCreateAllocator(&aci, &m_allocator);
    }

    Context::~Context() {
        m_main_pool.reset();
        m_transfer_pool.reset();
        m_compute_pool.reset();

        if (m_instance) {
            if (m_device)
                if (m_allocator) {
                    vmaDestroyAllocator(m_allocator);
                }

            if (m_pipeline_cache) {
                auto data = m_device.getPipelineCacheData(m_pipeline_cache);
                {
                    std::ofstream f("pipeline_cache", std::ios::binary | std::ios::out);
                    f.write(reinterpret_cast<const char *>(data.data()), data.size());

                    f.close();
                }

                m_device.destroy(m_pipeline_cache);
            }

            m_device.destroy();

            if (m_debug_messenger.has_value()) {
                m_instance.destroy(m_debug_messenger.value());
            }

            m_instance.destroy();
        }

        delete m_debug_user_data;
    }

    vk::Instance Context::instance() const {
        return m_instance;
    }

    std::optional<vk::DebugUtilsMessengerEXT> Context::debug_messenger() const {
        return m_debug_messenger;
    }

    vk::PhysicalDevice Context::physical_device() const {
        return m_physical_device;
    }

    vk::Device Context::device() const {
        return m_device;
    }

    vk::Queue Context::main_queue() const {
        return m_main_queue;
    }

    uint32_t Context::main_queue_family() const {
        return m_main_queue_family;
    }

    vk::Queue Context::transfer_queue() const {
        return m_transfer_queue;
    }

    uint32_t Context::transfer_queue_family() const {
        return m_transfer_queue_family;
    }

    vk::Queue Context::compute_queue() const {
        return m_compute_queue;
    }

    uint32_t Context::compute_queue_family() const {
        return m_compute_queue_family;
    }

    vk::PipelineCache Context::pipeline_cache() const {
        return m_pipeline_cache;
    }

    VmaAllocator Context::allocator() const {
        return m_allocator;
    }

    VmaAllocated<vk::Image> Context::allocate_image(const vk::ImageCreateInfo &ici, const VmaAllocationCreateInfo &allocation_create_info) const {
        VmaAllocated<vk::Image> res;

        VkImage img;

        VkImageCreateInfo ici_ = ici;
        vmaCreateImage(m_allocator, &ici_, &allocation_create_info, &img, &res.allocation, &res.allocation_info);
        res.resource = img;

        return res;
    }

    VmaAllocated<vk::Buffer> Context::allocate_buffer(const vk::BufferCreateInfo &bci, const VmaAllocationCreateInfo &allocation_create_info) const {
        VmaAllocated<vk::Buffer> res;

        VkBuffer buf;

        VkBufferCreateInfo bci_ = bci;

        vmaCreateBuffer(m_allocator, &bci_, &allocation_create_info, &buf, &res.allocation, &res.allocation_info);
        res.resource = buf;

        return res;
    }

    void Context::free_image(const VmaAllocated<vk::Image> &image) const {
        vmaDestroyImage(m_allocator, image.resource, image.allocation);
    }

    void Context::free_buffer(const VmaAllocated<vk::Buffer> &buffer) const {
        vmaDestroyBuffer(m_allocator, buffer.resource, buffer.allocation);
    }

    VmaAllocated<vk::Buffer> Context::allocate_gpu_buffer(size_t size, const void *data, vk::BufferUsageFlags usage) const {
        auto buf = allocate_buffer(vk::BufferCreateInfo{{}, size, usage | vk::BufferUsageFlagBits::eTransferDst}, VmaAllocationCreateInfo{{}, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE});

        if (data) {
            auto stage = allocate_staging_buffer(size, data, {});
            copy_buffer_to_buffer(stage, buf, size, 0, 0);
        }

        return buf;
    }

    VmaAllocated<vk::Buffer> Context::allocate_staging_buffer(size_t size, const void *data, vk::BufferUsageFlags usage) const {
        auto buf = allocate_buffer(vk::BufferCreateInfo{{}, size, usage | vk::BufferUsageFlagBits::eTransferSrc},
                                   VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO});
        if (data) {
            void *p = map_buffer(buf);
            std::memcpy(p, data, size);
            unmap_buffer(buf);
        }

        return buf;
    }

    VmaAllocated<vk::Buffer> Context::allocate_host_buffer(size_t size, const void *data, vk::BufferUsageFlags usage) const {
        auto buf = allocate_buffer(vk::BufferCreateInfo{{}, size, usage},
                                   VmaAllocationCreateInfo{VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_HOST});
        if (data) {
            void *p = map_buffer(buf);
            std::memcpy(p, data, size);
            unmap_buffer(buf);
        }

        return buf;
    }

    void *Context::map_buffer(const VmaAllocated<vk::Buffer> &buffer) const {
        void *p;
        vmaMapMemory(m_allocator, buffer.allocation, &p);
        return p;
    }

    void Context::unmap_buffer(const VmaAllocated<vk::Buffer> &buffer) const {
        vmaUnmapMemory(m_allocator, buffer.allocation);
    }

    void Context::copy_buffer_to_buffer(const VmaAllocated<vk::Buffer> &src, const VmaAllocated<vk::Buffer> &dst, vk::DeviceSize size, vk::DeviceSize src_offset,
                                        vk::DeviceSize dst_offset) const {
        vk::Fence         fence = m_device.createFence({});
        vk::CommandBuffer cmd   = m_transfer_pool->allocate_command_buffer();

        vk::BufferCopy copy{src_offset, dst_offset, size};

        cmd.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        cmd.copyBuffer(src.resource, dst.resource, copy);
        cmd.end();

        vk::SubmitInfo si{};
        si.setCommandBuffers(cmd);
        m_transfer_queue.submit(si, fence);
        auto _ = m_device.waitForFences(fence, true, UINT64_MAX);

        m_transfer_pool->free_command_buffers({cmd});
        m_device.destroy(fence);
    }

    void Context::setup(const std::shared_ptr<Context> &me) {
        m_main_pool = std::make_shared<CommandPool>(me, m_main_queue_family);
        m_transfer_pool = std::make_shared<CommandPool>(me, m_transfer_queue_family);
        m_compute_pool = std::make_shared<CommandPool>(me, m_compute_queue_family);
    }

    CommandPool::CommandPool(const std::shared_ptr<Context> &context, uint32_t queue_family, bool resettable) : m_context(context) {
        m_command_pool = m_context->device().createCommandPool({
            resettable ? vk::CommandPoolCreateFlagBits::eResetCommandBuffer : vk::CommandPoolCreateFlags{},
            queue_family,
        });
    }

    CommandPool::~CommandPool() {
        m_context->device().destroy(m_command_pool);
    }

    std::vector<vk::CommandBuffer> CommandPool::allocate_command_buffers(uint32_t count, vk::CommandBufferLevel level) const {
        return m_context->device().allocateCommandBuffers(vk::CommandBufferAllocateInfo{m_command_pool, level, count});
    }

    vk::CommandBuffer CommandPool::allocate_command_buffer(vk::CommandBufferLevel level) const {
        return m_context->device().allocateCommandBuffers(vk::CommandBufferAllocateInfo{m_command_pool, level, 1})[0];
    }

    void CommandPool::free_command_buffers(const std::vector<vk::CommandBuffer> &command_buffers) const {
        m_context->device().freeCommandBuffers(m_command_pool, command_buffers);
    }
} // namespace neuron
