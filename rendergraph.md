# Render Graph Design


Main class: `neuron::render::RenderGraph`

Render targets: `neuron::intfc::RenderTargetBase`
Kinds of render targets:
    DisplaySystem (or maybe frame info, leave the display system to manage its syncs)
    ImageSet


`Stage` represents a part of the render pass

`Pass` represents a render pass, comprised of (possibly) multiple stages

`RenderGraph` stores stages and resource dependencies

`ResourceDependency` represents a single resource dependency at a point in the graph



## Example

a very basic pass which just clears a single target with a color needs:
- the `RenderGraph`
  - a `Pass`
    - a `RenderingStage` (subtype of `Stage`), which contains the clear color and render area information.
  - extra dependencies:
    - from the start, implicitly generated dependency to ColorAttachmentOutput (because the first use of the target will be the rendering-stage) transitioning the image layout from Undefined to ColorAttachmentOptimal.
    - at the end, generated because the graph is indicated that the render target is required to be in PresentSrc layout at the end, dependency between ColorAttachmentOutput to BottomOfPipe.