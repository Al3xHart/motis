module Util.List exposing (elementAt, dropEnd, last, padListRight)


elementAt : List a -> Int -> Maybe a
elementAt list index =
    List.drop index list |> List.head


last : List a -> Maybe a
last =
    List.foldl (Just >> always) Nothing


dropEnd : Int -> List a -> List a
dropEnd n list =
    let
        f : a -> ( List a, Int ) -> ( List a, Int )
        f x ( result, n_ ) =
            if n_ <= 0 then
                ( x :: result, n_ )

            else
                ( result, n_ - 1 )

        ( final, _ ) =
            List.foldr f ( [], n ) list
    in
    final


padListRight : Int -> a -> List a -> List a
padListRight targetLen filler list =
    let
        listLen =
            List.length list
    in
    if listLen < targetLen then
        list ++ List.repeat (targetLen - listLen) filler

    else
        list
