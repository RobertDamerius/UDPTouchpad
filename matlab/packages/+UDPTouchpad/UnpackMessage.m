function [success,result] = UnpackMessage(bytes, numBytes)
    %UnpackMessage Unpack a binary message from the UDP Touchpad app.
    % 
    % INPUT
    % bytes    ... Array of bytes containing the message.
    % numBytes ... Number of bytes that represent the actual message.
    % 
    % RETURN
    % success  ... True if unpacking succeeded, false otherwise.
    % result   ... Output structure containing the unpacked data. All values are zero if unpacking failed.

    % set default output values
    success = false;
    result.counter         = uint8(0);
    result.screenWidth     = uint32(0);
    result.screenHeight    = uint32(0);
    result.pointerID       = zeros(10,1,'uint8');
    result.pointerPosition = zeros(2,10,'double');
    result.rotationVector  = zeros(3,1,'double');
    result.acceleration    = zeros(3,1,'double');
    result.angularRate     = zeros(3,1,'double');

    % check message length and header byte
    if((numBytes ~= 136) || (numel(bytes) < numBytes)), return; end
    if(uint8(bytes(1)) ~= uint8(0x42)), return; end

    % unpack data
    result.counter = uint8(bytes(2));
    result.screenWidth = unpack_ui32(bytes(3:6));
    result.screenHeight = unpack_ui32(bytes(7:10));
    result.pointerID = uint8(bytes(11:20));
    offset = 21;
    for k = 1:10
        result.pointerPosition(1,k) = unpack_f32(bytes(offset:offset+3));
        offset = offset + 4;
        result.pointerPosition(2,k) = unpack_f32(bytes(offset:offset+3));
        offset = offset + 4;
    end
    for k = 1:3
        result.rotationVector(k) = unpack_f32(bytes(offset:offset+3));
        offset = offset + 4;
    end
    for k = 1:3
        result.acceleration(k) = unpack_f32(bytes(offset:offset+3));
        offset = offset + 4;
    end
    for k = 1:3
        result.angularRate(k) = unpack_f32(bytes(offset:offset+3));
        offset = offset + 4;
    end
    success = true;
end

function u32 = unpack_ui32(bytes)
    u32 = uint32(typecast(uint8([bytes(4), bytes(3), bytes(2), bytes(1)]),'uint32'));
end

function f32 = unpack_f32(bytes)
    f32 = double(typecast(uint8([bytes(4), bytes(3), bytes(2), bytes(1)]),'single'));
end
