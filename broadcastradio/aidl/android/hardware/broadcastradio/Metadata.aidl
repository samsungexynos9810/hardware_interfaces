/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.hardware.broadcastradio;

/**
 * An element of metadata array.
 */
@VintfStability
@JavaDerive(equals=true, toString=true)
union Metadata {
    /**
     * RDS PS (string)
     */
    String rdsPs;

    /**
     * RDS PTY (uint8_t)
     */
    int rdsPty;

    /**
     * RBDS PTY (uint8_t)
     */
    int rbdsPty;

    /**
     * RDS RT (string)
     */
    String rdsRt;

    /**
     * Song title (string)
     */
    String songTitle;

    /**
     * Artist name (string)
     */
    String songArtist;

    /**
     * Album name (string)
     */
    String songAlbum;

    /**
     * Station icon (uint32_t, see {@link IBroadcastRadio#getImage})
     */
    int stationIcon;

    /**
     * Album art (uint32_t, see {@link IBroadcastRadio#getImage})
     */
    int albumArt;

    /**
     * Station name.
     *
     * This is a generic field to cover any radio technology.
     *
     * If the PROGRAM_NAME has the same content as DAB_*_NAME or RDS_PS,
     * it may not be present, to preserve space - framework must repopulate
     * it on the client side.
     */
    String programName;

    /**
     * DAB ensemble name (string)
     */
    String dabEnsembleName;

    /**
     * DAB ensemble name abbreviated (string).
     *
     * The string must be up to 8 characters long.
     *
     * If the short variant is present, the long (DAB_ENSEMBLE_NAME) one must be
     * present as well.
     */
    String dabEnsembleNameShort;

    /**
     * DAB service name (string)
     */
    String dabServiceName;

    /**
     * DAB service name abbreviated (see DAB_ENSEMBLE_NAME_SHORT) (string)
     */
    String dabServiceNameShort;

    /**
     * DAB component name (string)
     */
    String dabComponentName;

    /**
     * DAB component name abbreviated (see DAB_ENSEMBLE_NAME_SHORT) (string)
     */
    String dabComponentNameShort;
}
