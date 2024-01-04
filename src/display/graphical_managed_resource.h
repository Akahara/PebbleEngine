#pragma once

namespace pbl
{

/*
A do-nothing structure, simply used to mark subclasses as "managed",
that is, they can be passed by copy anywhere in the program but an
unmodifiable copy is conserved in the "manager", subclasses are not
responsible for the lifetime of their contents, the manager is.

ie. The texture manager produces textures, textures can be passed in
the program by copy but the underlying resource is only managed by
the texture manager (when a texture is destroyed it does not release
the resource, the texture manager does that)

Note that some subclasses are still passed by pointer because they
can retain/share state (ie. shaders or effects that cache binding
locations). In that case the pointer can be shared arround and used
until the manager is destroyed. It is NOT the responsibility of the
one that receives the pointer to delete the object.

ManagedResource contract:
- trivially constructible (holds no concrete handle)
- trivially destructible (the manager is responsible for the resource
  destruction)
- trivially copiable
- it and its content cannot be modified in any way shape or form,
  or it is explicitly stated that copied instances refer to the same
  resource and therefore can be mutated from anywhere at anytime
- when aquired by pointer, the pointer may be copied but not the value
*/
struct managed_resource {};

class GraphicalResourceRegistry;

}