// Original SoulSilver exports absent from the public Platinum interface.
void Billboard_SetResources(Billboard *billboard,const BillboardResources *resources)
{
    GF_ASSERT(billboard);
    SetAndAppendBillboard(billboard,resources);
}
NNSGfdTexKey Billboard_GetTexKey(const Billboard *billboard)
{
    GF_ASSERT(billboard);
    return billboard->texKey;
}
NNSGfdPlttKey Billboard_GetPlttKey(const Billboard *billboard)
{
    GF_ASSERT(billboard);
    return billboard->plttKey;
}
