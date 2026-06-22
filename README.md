# Renderer
Software renderer with AVX2<br><br>
It is build on tiled architecture where each screen section is independently rendered by separate thread.<br>
Fist command list is written and then executed in each thread independently.<br>
For thread synchronization only two std::barrier are used in the beginning and in the end of command list execution.<br>
For rasterization 8-lane AVX2 warps are used, organized in 4x2 pixel blocks/stamps.<br>
__m256 and __m128 are also used in several other places to accelerate computations.<br>
Renderer features trilinear filtering, alpha test and blending, tiled forward+ lighting with omni lights and directional light with shadow.<br>
On Alder Lake Intel Core i5 with 12 hardware threads I got average 60 fps with lighting mode on my test scene (resolution - 1280x720)<br>
Sponza performance though not so great, fps can drop to 15. But sponza has hell many vertices.<br><br>
Controls:<br>
&emsp;w - forward<br>
&emsp;s - backward<br>
&emsp;v - freeze view<br><br>
Render modes:<br>
&emsp;1 - wireframe<br>
&emsp;2 - textures only<br>
&emsp;3 - normals<br>
&emsp;4 - lighting
